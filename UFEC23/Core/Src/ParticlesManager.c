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
#include "cmsis_os.h"
#include "stm32f1xx_hal.h"
#include "EspBridge.h"
#include "DebugManager.h"
#include "ParticlesManager.h"
#include "MemBlock.h"

#define START_BYTE 0xCC
#define READ_CMD 0x00
#define WRITE_CMD 0xC0
#define FIRECNT_CMD 0x80
#define SETZERO_CMD 0x40
#define STOP_BYTE 0x99

#define RX_BUFFER_LENGTH 64
#define TX_BUFFER_LENGTH 20

extern UART_HandleTypeDef huart3;

static osSemaphoreId MP_UART_SemaphoreHandle;

static uint8_t RX_BUFFER[RX_BUFFER_LENGTH];
static uint8_t TX_BUFFER[TX_BUFFER_LENGTH];
static MeasureParticles_t ParticleDevice;
static uint16_t uartErrorCount = 0;
static bool particleBoardAbsent = false;
static bool config_mode = false;
static bool setZero = false;

bool validateRxChecksum(uint8_t buffer_index);
static bool is_part_data_updated(void);
static void update_part_variables(void);

// For debug purposes... Contains parameter modifications in ComputeParticleAdjustment
float algo_mod[4] = {0.0,0.0,0.0,0.0}; //

void ParticleInit(void)
{
	ParticleDevice.LED_current_meas = 0;
	ParticleDevice.ch0_ON = 0;
	ParticleDevice.ch0_OFF = 0;
	ParticleDevice.ch1_ON = 0;
	ParticleDevice.ch1_OFF = 0;
	ParticleDevice.variance = 0;
	ParticleDevice.temperature = 0;
	ParticleDevice.LED_current_meas = 0;
	ParticleDevice.slope = 0;
	ParticleDevice.Lux_ON = 0;
	ParticleDevice.Lux_OFF = 0;
	ParticleDevice.TimeSinceInit = 0;
	ParticleDevice.last_particle_time = 0;
	ParticleDevice.crit = 0.0;
	ParticleDevice.normalized_zero = 80.0;


}

void ParticlesManager(void const * argument) {

	osSemaphoreDef(MP_UART_SemaphoreHandle);
	MP_UART_SemaphoreHandle = osSemaphoreCreate(osSemaphore(MP_UART_SemaphoreHandle), 1);
	osSemaphoreWait(MP_UART_SemaphoreHandle,1); //decrement semaphore value for the lack of way to create a semaphore with a count of 0.
	const PF_PartParam* pParam = PB_GetParticlesParam();

	static bool Rx_complete = false;
	static bool rx_success = true;
	static uint16_t tx_checksum, rx_checksum;
	uint8_t rx_payload_size, tx_size, zero_current;
	uint32_t response_delay = 800;

	for(;;) {

		if(rx_success)
		{
			rx_success = false;
			particleBoardAbsent = false;
			uartErrorCount = 0;
			osDelay(2000);
		}
		if(particleBoardAbsent)
		{
			osDelay(30000); //Ping every 30 sec... see if back on line
		}

		if(config_mode)
		{
			TX_BUFFER[0] = START_BYTE;
			TX_BUFFER[1] = WRITE_CMD | 0x04;
			tx_checksum = TX_BUFFER[1];
			TX_BUFFER[2] = (uint8_t)pParam->s32TLSGAIN;
			TX_BUFFER[3] = (uint8_t)pParam->s32TSLINT;
			TX_BUFFER[4] = (uint8_t)pParam->s32DACCMD;
			TX_BUFFER[5] = (uint8_t)pParam->s32TIMEINTERVAL;
			for(uint8_t j = 2;j < 6;j++)
			{
				tx_checksum += TX_BUFFER[j];
			}
			TX_BUFFER[6] = (uint8_t)(tx_checksum >> 8);
			TX_BUFFER[7] = (uint8_t)(tx_checksum & 0x00FF);
			TX_BUFFER[8] = STOP_BYTE;
			tx_size = 9;
			response_delay = 600;
		}else if(setZero)
		{
			TX_BUFFER[0] = START_BYTE;
			TX_BUFFER[1] = SETZERO_CMD;
			tx_checksum = SETZERO_CMD;
			TX_BUFFER[2] = (uint8_t)(tx_checksum >> 8);
			TX_BUFFER[3] = (uint8_t)(tx_checksum & 0x00FF);
			TX_BUFFER[4] = STOP_BYTE;
			tx_size = 5;
			response_delay = 800;
		}else
		{
			TX_BUFFER[0] = START_BYTE;
			TX_BUFFER[1] = READ_CMD;
			tx_checksum = READ_CMD;
			TX_BUFFER[2] = (uint8_t)(tx_checksum >> 8);
			TX_BUFFER[3] = (uint8_t)(tx_checksum & 0x00FF);
			TX_BUFFER[4] = STOP_BYTE;
			tx_size = 5;
			response_delay = 800;


		}
		rx_payload_size = 0;
		HAL_UART_Transmit_IT(&huart3, TX_BUFFER, tx_size);

		if(osErrorOS == osSemaphoreWait(MP_UART_SemaphoreHandle,response_delay)) //wait for an answer or retry
		{
			//clearly something is wrong Abort the transmission
			//HAL_GPIO_WritePin(STATUS_LED0_GPIO_Port,STATUS_LED0_Pin,RESET);
			HAL_UART_Abort_IT(&huart3);
			HAL_UART_DeInit(&huart3);
			osDelay(100);
			MX_USART3_UART_Init();
			osDelay(100);
		}
		else
		{
			RX_BUFFER[0] = 0;
			RX_BUFFER[1] = 0;
			HAL_UARTEx_ReceiveToIdle_IT(&huart3, RX_BUFFER,RX_BUFFER_LENGTH);
			Rx_complete = false;
			config_mode = false;
			do{

				if(osErrorOS == osSemaphoreWait(MP_UART_SemaphoreHandle,400))
				{
					if(RX_BUFFER[0] == START_BYTE)
					{
						rx_payload_size = RX_BUFFER[1] & 0x3F;

						if(rx_payload_size != 0 && RX_BUFFER[rx_payload_size + 4] == STOP_BYTE)
						{
							Rx_complete = true;
						}

					}else
					{
						uartErrorCount++;
						if(uartErrorCount > 10)
						{
							particleBoardAbsent = true;
						}

					}
					break;

				}

			}while (!Rx_complete);

		}

		if(Rx_complete)
		{
			g_MEMBLOCK_sInstance.u32SensorParticleRXCount++;

			int sign = 1;
			rx_checksum = RX_BUFFER[1];
			for(uint8_t i = 2;i <= rx_payload_size+1;i++)
			{
				rx_checksum += RX_BUFFER[i];
			}

			if(rx_checksum == ((uint16_t)(RX_BUFFER[rx_payload_size+2] << 8) + (uint16_t)RX_BUFFER[rx_payload_size+3]))
			{
				rx_success = true;
				if((RX_BUFFER[1] & 0xC0) == READ_CMD)
				{
					ParticleDevice.ch0_ON = (uint16_t)(RX_BUFFER[2] << 8) + (uint16_t)RX_BUFFER[3];
					ParticleDevice.ch0_OFF = (uint16_t)(RX_BUFFER[4] << 8) + (uint16_t)RX_BUFFER[5];
					ParticleDevice.ch1_ON = (uint16_t)(RX_BUFFER[6] << 8) + (uint16_t)RX_BUFFER[7];
					ParticleDevice.ch1_OFF = (uint16_t)(RX_BUFFER[8] << 8) + (uint16_t)RX_BUFFER[9];
					ParticleDevice.variance = (uint16_t)(RX_BUFFER[10] << 8) + (uint16_t)RX_BUFFER[11];
					ParticleDevice.temperature = (uint16_t)(RX_BUFFER[12] << 8) + (uint16_t)RX_BUFFER[13];
					ParticleDevice.LED_current_meas = (uint16_t)(RX_BUFFER[14] << 8) + (uint16_t)RX_BUFFER[15];

					if(RX_BUFFER[16] & 0x80)
					{
						RX_BUFFER[16] &= 0x7F;
						sign = -1;
					}
					ParticleDevice.slope = sign*((int)(RX_BUFFER[16] << 8) + (int)RX_BUFFER[17]);
					ParticleDevice.Lux_ON = (uint16_t)(RX_BUFFER[18] << 8) + (uint16_t)RX_BUFFER[19];
					ParticleDevice.Lux_OFF = (uint16_t)(RX_BUFFER[20] << 8) + (uint16_t)RX_BUFFER[21];
					ParticleDevice.TimeSinceInit = (uint32_t)(RX_BUFFER[22] << 24) + (uint32_t)(RX_BUFFER[23] << 16) + (uint32_t)(RX_BUFFER[24] << 8) + (uint32_t)(RX_BUFFER[25]);
					update_part_variables();
				}else if((RX_BUFFER[1] & 0xC0) == WRITE_CMD)
				{
					//TODO: Implement config
					if(RX_BUFFER[2] == pParam->s32TLSGAIN && RX_BUFFER[3] == pParam->s32TSLINT
							&& RX_BUFFER[4] == pParam->s32DACCMD && RX_BUFFER[5] == pParam->s32TIMEINTERVAL)
					{
						config_mode = false;
					}

				}else if((RX_BUFFER[1] & 0xC0) == SETZERO_CMD)
				{
					setZero = false;
					ParticleDevice.zero = (uint16_t)(RX_BUFFER[4] << 8) + (uint16_t)RX_BUFFER[5];
					zero_current = RX_BUFFER[7];
					//ParticleDevice.normalized_zero = (float)ParticleDevice.zero/(float)zero_current;


				}
			}
		}

	}

}

uint16_t Particle_getCH0(void)
{
	return ParticleDevice.ch0_ON;
}

uint16_t Particle_getCH1(void)
{
	return ParticleDevice.ch1_ON;
}

uint16_t Particle_getCH0_OFF(void)
{
	return ParticleDevice.ch0_OFF;
}
uint16_t Particle_getCH1_OFF(void)
{
	return ParticleDevice.ch1_OFF;
}

uint16_t Particle_getVariance(void)
{
	return ParticleDevice.variance;
}

int Particle_getSlope(void)
{
	return ParticleDevice.slope;
}

float Particle_getZeroNorm(void)
{
	return ParticleDevice.normalized_zero;
}

uint16_t Particle_getTemperature(void)
{
	return (uint16_t)(ParticleDevice.temperature*9/50+32);
}

uint16_t Particle_getCurrent(void)
{
	return (uint16_t)(ParticleDevice.LED_current_meas*3.3/4.096);
}

uint16_t Particle_getLuxON(void)
{
	return ParticleDevice.Lux_ON;
}
uint16_t Particle_getLuxOFF(void)
{
	return ParticleDevice.Lux_OFF;
}

uint32_t Particle_getTime(void)
{
	return ParticleDevice.TimeSinceInit;
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


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{

	if(huart->Instance == USART3)
	{
		osSemaphoreRelease(MP_UART_SemaphoreHandle);
	}else if(huart->Instance == USART2)
	{
		osSemaphoreRelease(ESP_UART_SemaphoreHandle);
	}else if(huart->Instance == USART1)
	{
		set_sec_flag();
	}

}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{

	if(huart->Instance == USART3)
	{
		osSemaphoreRelease(MP_UART_SemaphoreHandle);
	}else if(huart->Instance == USART2)
	{
		osSemaphoreRelease(ESP_UART_SemaphoreHandle);
	}

}


void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	uint32_t errorcode;
	ErrorType error = UART;

	if(huart->Instance == USART3 || huart->Instance == USART2)
	{
		errorcode = huart->ErrorCode;//send this error code up the line to communicate to PC?
		setErrorFlag(errorcode, error);
	}

}
void Particle_setConfig(void)
{
	config_mode = true;
}

void Particle_requestZero(void)
{
	setZero = true;
}


static void update_part_variables(void)
{
	int std,slp;
	float crit = 0.0;

	std = ParticleDevice.variance < 80? (int)ParticleDevice.variance:80;

	if(ParticleDevice.slope > 0){
		slp = ParticleDevice.slope < 20? ParticleDevice.slope:20;
		crit = (float)(std+slp);
	}else
	{
		slp = abs(ParticleDevice.slope) < 20? ParticleDevice.slope:-20;
		crit = (float)(std-slp)*(-1);
	}

	ParticleDevice.crit =  crit/100;
	algo_mod[2] = .8*algo_mod[2]+ .2*ParticleDevice.crit;

	if(is_part_data_updated())
	{
		ParticleDevice.last_particle_time = ParticleDevice.TimeSinceInit;
	}

}

static bool is_part_data_updated(void)
{
	return ParticleDevice.last_particle_time != ParticleDevice.TimeSinceInit;
}



bool computeParticleLowAdjustment(float dTavant, int* delta, float* speed, uint32_t Time_ms,
		int32_t baffleTemperature, int32_t temperature_limit, bool* OpenGrill)
{
	// Function memory variables (pour timings et validation de la pertinence des corrections précédentes)
	static uint32_t lastTimeInFunc = 0;
	static uint32_t TimeOfMajorCorrection = 0;

	// Variables for decicison making
	const PF_PartParam* pParam = PB_GetParticlesParam();
	bool crit_correction = false; // to avoid cancelling crit based correction with a diff based correction
	static int MajorCorrection_counter = 0;  // after 2 or 3, change state to superlow or allow grid to open
	int32_t aperture = 0;
	float Sec_per_step = 0.0;

	float diff = (float)(10*ParticleDevice.ch0_ON/ParticleDevice.LED_current_meas) - ParticleDevice.normalized_zero;

	if(Time_ms - TimeOfMajorCorrection > SECONDS(pParam->s32MAJ_CORR_INTERVAL))
	{
		if((baffleTemperature - temperature_limit) < -pParam->s32TBUF_FLOSS)
		{
			if(dTavant < -1*pParam->s32DT_THRESHOLD_H/60.0)
			{
				aperture = 5;
				*OpenGrill = true;
				Sec_per_step = 0;

			}else
			{
				aperture = 10;
				Sec_per_step = 0.5;
			}

		TimeOfMajorCorrection = Time_ms;
		MajorCorrection_counter++;

		}else
		{
			MajorCorrection_counter = 0;
		}
	}

	if(ParticleDevice.crit > pParam->s32CRIT_THRESHOLD_L/100 && MajorCorrection_counter == 0)
	{
		if((baffleTemperature - temperature_limit) < pParam->s32TBUF_WORKRANGE)
		{
			aperture = 5;
			Sec_per_step = 0.5;
		}else if((baffleTemperature - temperature_limit) < pParam->s32TBUF_OVERHEAT)
		{
			if(dTavant >= 0)
			{
				aperture = -5;
				Sec_per_step = 1;
			}else
			{
				aperture = 5;
				Sec_per_step = 1;
			}

		}else
		{
			aperture = -10;
			Sec_per_step = 1;
		}
		crit_correction = true;

	}

	if(!crit_correction && MajorCorrection_counter == 0)
	{
		if((baffleTemperature - temperature_limit) > pParam->s32TBUF_FLOSS)
		{
			if(diff > pParam->s32DIFF_TRESHOLD_H)
			{
				aperture = -5;
				Sec_per_step = .5;
			}else if(diff > pParam->s32DIFF_TRESHOLD_L)
			{
				aperture = -1;
				Sec_per_step = 3;
			}
		}

	}



	//algo_mod[2] = .8*algo_mod[2]+ .2*ParticleDevice.crit;


	*delta = aperture;
	*speed = Sec_per_step;

	////////////////////////////////////////////
	if(Time_ms > (lastTimeInFunc + SECONDS(5)))
	{
		lastTimeInFunc = Time_ms;
		algo_mod[0] = aperture;
		algo_mod[1] = Sec_per_step;
		algo_mod[3] = dTavant;
	}
	///////////////////////////////////////////

	// If too many consecutive major corrections, go to comb_superlow
	if(MajorCorrection_counter != 0){
		return false;
	}
	return true;


}

bool computeParticleCombLowAdjustment(float dTbaffle, uint32_t Time_ms, int32_t baffleTemperature,
							int32_t temperature_limit, ParticleAction* action)
{
	const PF_PartParam* pParam = PB_GetParticlesParam();
	static uint32_t TimeOfLastCorrection = 0;
	static uint32_t delay = 0;

	//algo_mod[2] = .8*algo_mod[2]+ .2*ParticleDevice.crit;
	float diff = (float)(10*ParticleDevice.ch0_ON/ParticleDevice.LED_current_meas) - ParticleDevice.normalized_zero;

	switch(*action)
	{
	case FAST_CLOSE:
	case FAST_OPEN:
		delay = (uint32_t)pParam->s32FAST_CORR_INTERVAL;
		break;
	case AVG_OPEN:
	case AVG_CLOSE:
		delay = (uint32_t)pParam->s32AVG_CORR_INTERVAL;
		break;
	case SLOW_OPEN:
	case SLOW_CLOSE:
		delay = (uint32_t)pParam->s32SLOW_CORR_INTERVAL;
		break;
	case VERY_SLOW_CLOSE:
	case VERY_SLOW_OPEN:
		delay = (uint32_t)pParam->s32VERY_SLOW_CORR_INTERVAL;
		break;
	case INSTANT_CLOSE:
	case INSTANT_OPEN:
		delay = (uint32_t)pParam->s32MAJ_CORR_INTERVAL;
	default:
		break;
	}

	if(Time_ms < (TimeOfLastCorrection + SECONDS(delay)))
	{
		*action = WAIT;
		return false;
	}
	TimeOfLastCorrection = Time_ms;
	delay = 0;

	if(baffleTemperature - temperature_limit < -1*pParam->s32TBUF_FLOSS)
	{
		if(dTbaffle > -1.0*(float)pParam->s32DT_THRESHOLD_L/60.0)
		{
			*action = WAIT;
			return true;
		}

		if(dTbaffle > -1.0*(float)pParam->s32DT_THRESHOLD_H/60.0)
		{
			*action = SLOW_OPEN;
			return true;
		}

		if(diff > pParam->s32DIFF_TRESHOLD_H)
		{
			*action = INSTANT_OPEN;
			return false;

		}

		*action =  FAST_OPEN;
		return false;
	}

	if(baffleTemperature - temperature_limit > pParam->s32TBUF_WORKRANGE)
	{
		if(dTbaffle > (float)pParam->s32DT_THRESHOLD_H/60.0)
		{
			*action =  FAST_CLOSE;
			return true;
		}

		if(dTbaffle > -1.0*(float)pParam->s32DT_THRESHOLD_L/60.0)
		{
			*action =  SLOW_CLOSE;
			return true;
		}

		*action =  WAIT;
		return true;

	}

	if(fabs(dTbaffle) < (float)pParam->s32DT_THRESHOLD_L/60.0)
	{
		*action =  VERY_SLOW_CLOSE;
		return true;
	}

	*action =  WAIT;
	return true;

}


ParticleAction computeParticleRiseAdjustment(float dTbaffle, uint32_t Time_ms, int32_t baffleTemperature)
{

		// Variables for decicison making
		const PF_PartParam* pParam = PB_GetParticlesParam();
		static uint32_t TimeOfMajorCorrection = 0;
		float diff = (float)(10*ParticleDevice.ch0_ON/ParticleDevice.LED_current_meas) - ParticleDevice.normalized_zero;

		if(Time_ms < (TimeOfMajorCorrection + MINUTES(1)))
		{
			return WAIT;
		}

		if(baffleTemperature > pParam->s32T_KIP_RISE)
		{
			if((diff > pParam->s32DIFF_TRESHOLD_H) && (dTbaffle > (float)pParam->s32DT_RISE/60.0)) //
			{
				TimeOfMajorCorrection = Time_ms;
				return FAST_CLOSE;
			}
		}

		if(dTbaffle < (float)pParam->s32DT_THRESHOLD_L/60.0)
		{
			return WAIT;
		}



		return SLOW_CLOSE;
}

// for DebugManager
float* get_algomod(void)
{
	return algo_mod;
}
