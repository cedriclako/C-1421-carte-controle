/*
Solutions Novika.
Copyright 2021
===========================================================================

Filename :      ParticlesManager.c

Author(s):      Charles Richard, CPI # 6045522

Public prefix : ParticlesManager

Project # : C-1421

Product: UFEC23

Creation date:  2022/10/12

Description:    Communication channel with particles measurement board

===========================================================================

Modifications
-------------

By    |   Date     | Version | Description
------+------------+---------+---------------------------------------------
CR    | 2022/10/12 | -       | Creation
===========================================================================
*/

#include "main.h"
#include "ParamFile.h"
#include "algo.h"
#include "stm32f1xx_hal.h"
#include "EspBridge.h"
#include "DebugManager.h"
#include "ParticlesManager.h"

typedef enum
{
	Idle = 0,
	Send_request,
	Request_sent,
	Validate_data,
	Data_ready
}Part_States;

#define START_BYTE 0xCC
#define READ_CMD 0x00
#define WRITE_CMD 0xC0
#define FIRECNT_CMD 0x80
#define SETZERO_CMD 0x40
#define STOP_BYTE 0x99

#define RX_BUFFER_LENGTH 64
#define TX_BUFFER_LENGTH 20

#define COMM_ERR_LIMIT   1000
#define TIME_TO_WAIT_IF_OK 2
#define TIME_TO_WAIT_IF_ERR 30

extern UART_HandleTypeDef huart3;

static Part_States currentState;
static Part_States nextState;
static uint8_t RX_BUFFER[RX_BUFFER_LENGTH];
static uint8_t TX_BUFFER[TX_BUFFER_LENGTH];
static MeasureParticles_t ParticleDevice;
static bool particleBoardAbsent = false;
static bool config_mode = false;
static bool setZero = false;
static bool IncFireCount = false;

bool validateRxChecksum(uint8_t buffer_index);

uint16_t Particle_Send_CMD(uint8_t cmd);


void Particle_Init(void)
{
	ParticleDevice.fLED_current_meas = 0;
	ParticleDevice.u16ch0_ON = 0;
	ParticleDevice.u16ch0_OFF = 0;
	ParticleDevice.u16ch1_ON = 0;
	ParticleDevice.u16ch1_OFF = 0;
	ParticleDevice.u16stDev = 0;
	ParticleDevice.u16temperature = 0;
	ParticleDevice.fLED_current_meas = 0;
	ParticleDevice.fslope = 0;
	ParticleDevice.u16Lux_ON = 0;
	ParticleDevice.u16Lux_OFF = 0;
	ParticleDevice.u16TimeSinceInit = 0;
	ParticleDevice.u16Last_particle_time = 0;
	ParticleDevice.fnormalized_zero = 80.0;

	currentState = Idle;
	nextState = Idle;
}

void ParticlesManager(uint32_t u32Time_ms)
{
	static uint16_t uartErrorCount = 0;
	static uint16_t tx_checksum, rx_checksum;
	static uint8_t rx_payload_size, tx_size;
	static uint32_t response_delay = 800;
	static uint8_t request_interval = TIME_TO_WAIT_IF_OK;
	static uint32_t u32LastReqTime = 0;
	int slp_sign = 1;


	switch(currentState)
	{
	case Idle:
		if(u32Time_ms - u32LastReqTime > SECONDS(request_interval))
		{
			//GC 2023-07-19 debug
			if(config_mode)
			{
				//Test unitaire - WRITE CMD
				TX_BUFFER[0] = START_BYTE;
				TX_BUFFER[1] = WRITE_CMD | 0x04; //Attention, devrait etre 0x05, updater aussi PF_VvalidatateConfig
				tx_checksum = TX_BUFFER[1];
				TX_BUFFER[2] = 3;//(uint8_t)pParam->s32TLSGAIN;
				TX_BUFFER[3] = 5;//(uint8_t)pParam->s32TSLINT;
				TX_BUFFER[4] = 7;//(uint8_t)pParam->s32DACCMD;
				TX_BUFFER[5] = 10;//(uint8_t)pParam->s32TIMEINTERVAL;  //GC Attention, devrait être un WORD (2 byte)
				//TX_BUFFER[6] = 0;//(uint8_t)pParam->s32TIMEINTERVAL;  //GC Attention, devrait être un WORD (2 byte)
				for(uint8_t j = 2;j < 6;j++)
				{
					tx_checksum += TX_BUFFER[j];
				}
				TX_BUFFER[6] = (uint8_t)(tx_checksum >> 8);
				TX_BUFFER[7] = (uint8_t)(tx_checksum & 0x00FF);
				TX_BUFFER[8] = STOP_BYTE;
				tx_size = 9;
				response_delay = 600;
			}else if(IncFireCount)
			{
				//Test unitaire - FIRECNT_CMD
				tx_checksum = Particle_Send_CMD(FIRECNT_CMD);
				tx_size = 5;
				response_delay = 600;
			}else if(setZero)
			{
				//Test unitaire - SETZERO CMD
				tx_checksum = Particle_Send_CMD(SETZERO_CMD);
				tx_size = 5;
				response_delay = 600;

			}else{

				//Test unitaire - READ_CMD
				tx_checksum = Particle_Send_CMD(READ_CMD);
				tx_size = 5;
				response_delay = 800;

				//TX_BUFFER[0] = START_BYTE;
				//TX_BUFFER[1] = READ_CMD;
				//tx_checksum = READ_CMD;
				//TX_BUFFER[2] = (uint8_t)(tx_checksum >> 8);
				//TX_BUFFER[3] = (uint8_t)(tx_checksum & 0x00FF);
				//TX_BUFFER[4] = STOP_BYTE;
				//tx_size = 5;
				//response_delay = 800;
			}
			nextState = Send_request;
		}
		break;
	case Send_request:
		if(uartErrorCount > COMM_ERR_LIMIT && request_interval !=TIME_TO_WAIT_IF_ERR)
		{
			request_interval = TIME_TO_WAIT_IF_ERR;
			nextState = Idle;
			break;
		}


		HAL_UART_Transmit_IT(&huart3, TX_BUFFER, tx_size);
		RX_BUFFER[0] = 0;
		u32LastReqTime = u32Time_ms;
		nextState = Request_sent;
		break;
	case Request_sent:
		HAL_UARTEx_ReceiveToIdle_IT(&huart3, RX_BUFFER,RX_BUFFER_LENGTH);
		if(RX_BUFFER[0] == START_BYTE)
		{
			rx_payload_size = RX_BUFFER[1] & 0x3F;

			if(rx_payload_size != 0 && RX_BUFFER[rx_payload_size + 4] == STOP_BYTE)
			{
				nextState = Validate_data;
			}

		}

		if(u32Time_ms - u32LastReqTime > response_delay)
		{
			if(uartErrorCount <= COMM_ERR_LIMIT)
			{
				nextState = Send_request;
			}else
			{
				nextState = Idle;
			}
			uartErrorCount++;

		}

		break;
	case Validate_data:
		rx_checksum = RX_BUFFER[1];
		for(uint8_t i = 2;i <= rx_payload_size+1;i++)
		{
			rx_checksum += RX_BUFFER[i];
		}

		if(rx_checksum == ((uint16_t)(RX_BUFFER[rx_payload_size+2] << 8) + (uint16_t)RX_BUFFER[rx_payload_size+3]))
		{
			particleBoardAbsent = false;
			request_interval = TIME_TO_WAIT_IF_OK;
			uartErrorCount = 0;
			nextState = Data_ready;
		}else
		{
			if(uartErrorCount <= COMM_ERR_LIMIT)
			{
				nextState = Send_request;
			}else
			{
				nextState = Idle;
			}
			uartErrorCount++;
		}
		break;
	case Data_ready:
		nextState = Idle;
		u32LastReqTime = u32Time_ms;
		if((RX_BUFFER[1] & 0xC0) == READ_CMD)
		{
			ParticleDevice.u16ch0_ON = (uint16_t)(RX_BUFFER[2] << 8) + (uint16_t)RX_BUFFER[3];
			ParticleDevice.u16ch0_OFF = (uint16_t)(RX_BUFFER[4] << 8) + (uint16_t)RX_BUFFER[5];
			ParticleDevice.u16ch1_ON = (uint16_t)(RX_BUFFER[6] << 8) + (uint16_t)RX_BUFFER[7];
			ParticleDevice.u16ch1_OFF = (uint16_t)(RX_BUFFER[8] << 8) + (uint16_t)RX_BUFFER[9];
			ParticleDevice.u16stDev = (uint16_t)(RX_BUFFER[10] << 8) + (uint16_t)RX_BUFFER[11];
			ParticleDevice.u16temperature = (uint16_t)(RX_BUFFER[12] << 8) + (uint16_t)RX_BUFFER[13];
			ParticleDevice.fLED_current_meas = P2F1DEC(((uint16_t)(RX_BUFFER[14] << 8) + (uint16_t)RX_BUFFER[15]));

			if(RX_BUFFER[16] & 0x80)
			{
				RX_BUFFER[16] &= 0x7F;
				slp_sign = -1;
			}
			ParticleDevice.fslope = P2F1DEC(slp_sign*((int)(RX_BUFFER[16] << 8) + (int)(uint8_t)RX_BUFFER[17]));
			ParticleDevice.u16Lux_ON = (uint16_t)(RX_BUFFER[18] << 8) + (uint16_t)RX_BUFFER[19];
			ParticleDevice.u16Lux_OFF = (uint16_t)(RX_BUFFER[20] << 8) + (uint16_t)RX_BUFFER[21];
			ParticleDevice.u16TimeSinceInit = (uint32_t)(RX_BUFFER[22] << 24) + (uint32_t)(RX_BUFFER[23] << 16) + (uint32_t)(RX_BUFFER[24] << 8) + (uint32_t)(RX_BUFFER[25]);

			ParticleDevice.fparticles = (float)ParticleDevice.u16ch0_ON/ParticleDevice.fLED_current_meas - ParticleDevice.fnormalized_zero;//TODO:comment

			config_mode = false; //GC 2023-07-19 Debug comm
		}else if((RX_BUFFER[1] & 0xC0) == WRITE_CMD)
		{
			//TODO: Implement config
			//if(RX_BUFFER[2] == pParam->s32TLSGAIN && RX_BUFFER[3] == pParam->s32TSLINT
			//		&& RX_BUFFER[4] == pParam->s32DACCMD && RX_BUFFER[5] == pParam->s32TIMEINTERVAL)
			//{
			//	config_mode = false;
			//}
			config_mode = false; //GC 2023-07-19 debug

		}else if((RX_BUFFER[1] & 0xC0) == SETZERO_CMD)
		{
			setZero = false;
			ParticleDevice.u16zero = (uint16_t)(RX_BUFFER[4] << 8) + (uint16_t)RX_BUFFER[5];
			//zero_current = RX_BUFFER[7];
			ParticleDevice.fnormalized_zero = (float)ParticleDevice.u16zero/((float)RX_BUFFER[7]/10);
		}else
		{
			if(uartErrorCount <= COMM_ERR_LIMIT)
			{
				nextState = Send_request;
			}else
			{
				nextState = Idle;
			}
			uartErrorCount++;
		}
		break;
	}
	if(nextState != currentState)
	{
		currentState = nextState;
	}

}


bool validateRxChecksum(uint8_t buffer_index)
{
	uint16_t sum = 0;
	uint8_t i;

	for(i = 1;i < buffer_index - 3;i++)
	{
		sum += RX_BUFFER[i];
	}

	return (sum == (uint16_t)(RX_BUFFER[i] << 8) + (uint16_t)RX_BUFFER[i+1]);
}

bool PM_isPboard_absent(void)
{
	return particleBoardAbsent;
}


void Particle_setConfig(void)
{
	config_mode = true;
}

void Particle_requestZero(void)
{
	setZero = true;
}

void Particle_IncFireCount(void)
{
	IncFireCount = true;
}

uint16_t Particle_Send_CMD(uint8_t cmd)
{
	static uint16_t tx_checksum;

	TX_BUFFER[0] = START_BYTE;
	TX_BUFFER[1] = cmd;
	tx_checksum = cmd;
	TX_BUFFER[2] = (uint8_t)(tx_checksum >> 8);
	TX_BUFFER[3] = (uint8_t)(tx_checksum & 0x00FF);
	TX_BUFFER[4] = STOP_BYTE;

	return tx_checksum;
}

const MeasureParticles_t* ParticlesGetObject(void)
{
	return &ParticleDevice;
}
