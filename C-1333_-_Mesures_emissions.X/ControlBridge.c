/*
Solutions Novika.
Copyright 2022
===========================================================================

Filename :      ControlBridge.c

Author(s):      Charles Richard, CPI # 6045522   

Public prefix : control

Project # : C-1421

Product: Mesure d'émissions polluantes

Creation date:  2022/10/14

Description:    Communication bridge between particles measurement and control card

===========================================================================

Modifications
-------------

By    |   Date     | Version | Description
------+------------+---------+---------------------------------------------
CR    | 2022/10/14 | 0.1      | Creation
===========================================================================
*/ 

#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "mcc_generated_files/uart2.h"
#include "mcc_generated_files/tmr1.h"
#include "TSL2591.h"
#include "ControlBridge.h"


#define MAX_BUFFER_SIZE 30

#define START_BYTE 0xCC
#define STOP_BYTE 0x99

#define WRITE_CMD 0xC0
#define READ_CMD 0x00

#define DATA_MEMORY_DEPTH 10

typedef enum
{
    eBridge_IDLE = 0,
    eBridge_PROCESSING_DATA,
    eBridge_RECEIVE_CMD,
    eBridge_RECEIVE_PAYLOAD,
    eBridge_RECEIVE_CHECKSUM,
    eBridge_RECEIVE_STOP,
    eBridge_VERIFY_CHECKSUM,
    eBridge_TRANSMIT_DATA,
    eBridge_VERIFY_PAYLOAD,
    eBridge_SET_CONFIG,
    eBridge_TRANSMIT_CONFIG,
    eBridge_COMM_ERROR,
            
    eBridge_NB_STATES
            
}EControlBridgeStates;

typedef struct {
    uint16_t CH0_ON;
    uint16_t CH1_ON;
    uint16_t CH0_OFF;
    uint16_t CH1_OFF;
    uint16_t Lux_ON;
    uint16_t Lux_OFF;
    
    uint16_t CH0_BUFFER[DATA_MEMORY_DEPTH];
    
    uint16_t particles;
	uint16_t variance;
	uint16_t temperature;
    uint32_t time_since_beginning;
	int slope;
    uint16_t LED_current_meas;

	uint16_t time_window;
	uint8_t TSL_gain;
	uint8_t TSL_integration_time;
    uint8_t DacCommand;
	
    
}EControlBridgeObject;

void bridgeTimerElapsed(void);
void updateCalculatedValues(void);

static EControlBridgeStates BridgeState;
static EControlBridgeObject bOBJ;
static bool DRDY = false;
static bool CTRL_REQ = false;
static uint8_t TX_BUFFER[MAX_BUFFER_SIZE];
void sendData(uint8_t packet_size);
void sendConfig(uint8_t packet_size);
uint8_t fillBuffer(void);
bool bridgeValidateConfig(uint8_t* payload,uint8_t payload_size);

void ControlBridgeInitialize(void)
{
    TMR1_SetInterruptHandler(bridgeTimerElapsed);
    
    
    bOBJ.CH0_ON = 0;
    bOBJ.CH1_ON = 0;
    bOBJ.CH0_OFF = 0;
    bOBJ.CH1_OFF = 0;
    bOBJ.variance = 0;
    bOBJ.temperature = 0;

    bOBJ.LED_current_meas = 0;
    bOBJ.time_window = 0;
    bOBJ.slope = 0;
    
    bOBJ.Lux_ON = 0;
    bOBJ.Lux_OFF = 0;
    
    /*
    bOBJ.CH0_ON = 0x1234;
    bOBJ.CH0_OFF = 0x5678;
    bOBJ.CH1_ON = 0x9ABC;
    bOBJ.CH1_OFF = 0xDEF1;
    bOBJ.variance = 0x2345;
    bOBJ.temperature = 0x6789;

    bOBJ.LED_current_meas = 0xABCD;
    bOBJ.time_window = 0xEF12;
    */
}

    
void ControlBridgeProcess(void)
{
    static uint8_t command, payload_size, i, sum_LSB, sum_MSB;
    static uint8_t payload[MAX_BUFFER_SIZE], tx_size;
    static uint16_t rx_checksum, temp_sum;
    static EControlBridgeStates entry_state;
   
    switch(BridgeState)
    {
        case eBridge_IDLE:
            if(UART2_is_rx_ready())
            {
                command = 0x80;
                rx_checksum = 0;
                payload_size = 0;
                sum_MSB = 0xFF;
                sum_LSB = 0;
                
                if(UART2_Read() == START_BYTE)
                {
                    TMR1_StartTimer();
                    BridgeState = eBridge_RECEIVE_CMD;
                    //printf("Start byte received\r\n");
                }
            }
            else if(DRDY)
            {
                BridgeState = eBridge_PROCESSING_DATA;
            }
            break;
        case eBridge_PROCESSING_DATA:
            if(!DRDY)
            {
                updateCalculatedValues();
                BridgeState = eBridge_IDLE;
            }
            break;
        case eBridge_RECEIVE_CMD:
            if(UART2_is_rx_ready())
            {
                command = UART2_Read();
                temp_sum = command;
                
                if((command & 0xC0) == READ_CMD)
                {
                    entry_state = eBridge_RECEIVE_CMD;
                    BridgeState = eBridge_RECEIVE_CHECKSUM;
                    //printf("Read Command received\r\n");
                }else if((command & 0xC0) == WRITE_CMD)
                {
                    payload_size = command & 0x3F;
                    i = 0;
                    BridgeState = eBridge_RECEIVE_PAYLOAD;
                    //printf("Write command received  payload = %u\r\n",payload_size);
                }else
                {
                    BridgeState = eBridge_COMM_ERROR;
                    printf("Invalid command received\r\n");
                }
            }
            break;
        case eBridge_RECEIVE_PAYLOAD:
            if(i < payload_size)
            {
                if(UART2_is_rx_ready())
                {
                    payload[i] = UART2_Read();
                    temp_sum += payload[i];
                    i++;
                }
            }else
            {
                entry_state = eBridge_RECEIVE_PAYLOAD;
                BridgeState = eBridge_RECEIVE_CHECKSUM;
                //printf("Payload completely received\r\n");
            }
            break;
        case eBridge_RECEIVE_CHECKSUM:
            if(UART2_is_rx_ready())
            {
                if(sum_MSB == 0xFF)
                {
                    sum_MSB = UART2_Read();
                }else
                {
                    sum_LSB = UART2_Read();
                    rx_checksum = (((uint16_t)sum_MSB) << 8)+(uint16_t)sum_LSB;
                    BridgeState = eBridge_RECEIVE_STOP; 
                    //printf("Checksum received = %u\r\n", rx_checksum);
                }
            }
            break;
        case eBridge_RECEIVE_STOP:
            if(UART2_is_rx_ready())
            {
                if(UART2_Read() == STOP_BYTE)
                {
                    TMR1_StopTimer();
                    BridgeState = eBridge_VERIFY_CHECKSUM;
                    //printf("Stop byte received\r\n");
                }else
                {
                    BridgeState = eBridge_COMM_ERROR;
                }
            }
        case eBridge_VERIFY_CHECKSUM:
            if(temp_sum == rx_checksum)
            {
                switch(entry_state)
                {
                    case eBridge_RECEIVE_CMD:
                        //printf("Transmitting data\r\n");
                        BridgeState = eBridge_TRANSMIT_DATA;
                        break;
                    case eBridge_RECEIVE_PAYLOAD:
                        BridgeState = eBridge_VERIFY_PAYLOAD;
                        break;
                    default:
                        BridgeState = eBridge_COMM_ERROR;
                        break;
                }
            }else   //abort transmission
            {
                BridgeState = eBridge_COMM_ERROR;
            }
            break;
        case eBridge_TRANSMIT_DATA:
            tx_size = fillBuffer();
            sendData(tx_size);
            BridgeState = eBridge_IDLE;
            break;
        case eBridge_VERIFY_PAYLOAD:
            printf("Verifiyng payload\r\n");
            if(bridgeValidateConfig(payload, payload_size))
            {
                BridgeState = eBridge_SET_CONFIG;
                printf("Payload verified\r\n");
                measureParticlesRequestReconfigure();
            }
            else
            {
                BridgeState = eBridge_COMM_ERROR;
            }
            break;
        case eBridge_SET_CONFIG:
    
            if(measureParticlesReadyForConfig())
            {
                measureParticlesSetParameters(bOBJ.DacCommand,(uint16_t)bOBJ.time_window*1000);
                TSLset_parameters(bOBJ.TSL_gain,bOBJ.TSL_integration_time);
                BridgeState = eBridge_TRANSMIT_CONFIG;
            }
            break;
        case eBridge_TRANSMIT_CONFIG:
            tx_size = 4;
            sendConfig(tx_size);
            BridgeState = eBridge_IDLE;
            break;
        case eBridge_COMM_ERROR: // Can add some diagnosis if wanted
            TMR1_StopTimer();
            printf("COMM ERROR \r\n");
            BridgeState = eBridge_IDLE;
            break;
        default:
            break;
    }
}

uint8_t fillBuffer(void)
{
    int s_sign = 1;
    int slope_cont = 0;
    TX_BUFFER[0] = (uint8_t)(bOBJ.CH0_ON >> 8);
    TX_BUFFER[1] = (uint8_t)(bOBJ.CH0_ON & 0x00FF);
    TX_BUFFER[2] = (uint8_t)(bOBJ.CH0_OFF >> 8);
    TX_BUFFER[3] = (uint8_t)(bOBJ.CH0_OFF & 0x00FF);
    TX_BUFFER[4] = (uint8_t)(bOBJ.CH1_ON >> 8);
    TX_BUFFER[5] = (uint8_t)(bOBJ.CH1_ON & 0x00FF);
    TX_BUFFER[6] = (uint8_t)(bOBJ.CH1_OFF >> 8);
    TX_BUFFER[7] = (uint8_t)(bOBJ.CH1_OFF & 0x00FF);
    TX_BUFFER[8] = (uint8_t)(bOBJ.variance >> 8);
    TX_BUFFER[9] = (uint8_t)(bOBJ.variance & 0x00FF);
    TX_BUFFER[10] = (uint8_t)(bOBJ.temperature >> 8);
    TX_BUFFER[11] = (uint8_t)(bOBJ.temperature & 0x00FF);
    TX_BUFFER[12] = (uint8_t)(bOBJ.LED_current_meas >> 8);
    TX_BUFFER[13] = (uint8_t)(bOBJ.LED_current_meas & 0x00FF);
    if(bOBJ.slope < 0)
    {
        s_sign = -1;
    }
    slope_cont = s_sign*bOBJ.slope;
    TX_BUFFER[14] = ((uint8_t)(slope_cont >> 8) ^ (s_sign & 0x80));
    TX_BUFFER[15] = (uint8_t)(slope_cont & 0x00FF);
    TX_BUFFER[16] = (uint8_t)(bOBJ.Lux_ON >> 8);
    TX_BUFFER[17] = (uint8_t)(bOBJ.Lux_ON & 0x00FF);
    TX_BUFFER[18] = (uint8_t)(bOBJ.Lux_OFF >> 8);
    TX_BUFFER[19] = (uint8_t)(bOBJ.Lux_OFF & 0x00FF);
    TX_BUFFER[20] = (uint8_t)(bOBJ.time_since_beginning >> 24);
    TX_BUFFER[21] = (uint8_t)(bOBJ.time_since_beginning >> 16);
    TX_BUFFER[22] = (uint8_t)(bOBJ.time_since_beginning >> 8);
    TX_BUFFER[23] = (uint8_t)(bOBJ.time_since_beginning & 0x000000FF);
    
//    printf("%u, %u, %u, %u, %u, %u, %u, %i \r\n",bOBJ.CH0_ON, bOBJ.CH0_OFF, bOBJ.CH1_ON,bOBJ.CH1_OFF,bOBJ.variance,bOBJ.temperature,bOBJ.LED_current_meas,bOBJ.slope);
    printf("%u, %u, %u, %i \r\n",bOBJ.CH0_ON,bOBJ.variance,bOBJ.LED_current_meas,bOBJ.slope);

    return 24;
}


void sendData(uint8_t packet_size)
{
    uint16_t checksum;
    uint8_t command_line;
    
    command_line = (uint8_t) (READ_CMD ^ packet_size);
    checksum = command_line;

    UART2_Write((uint8_t)START_BYTE);
    UART2_Write(command_line);
    
    for(uint8_t i = 0;i < packet_size;i++)
    {
        UART2_Write(TX_BUFFER[i]);
        checksum += TX_BUFFER[i];    
    }
    UART2_Write((uint8_t)(checksum >> 8));
    UART2_Write((uint8_t)(checksum & 0x00FF));  
    UART2_Write((uint8_t)STOP_BYTE);
//    printf("Output checksum = %u\r\n",checksum);
    
}

void sendConfig(uint8_t packet_size)
{
    uint16_t checksum;
    uint8_t command_line;
    
    command_line = (uint8_t) (WRITE_CMD ^ packet_size);

    UART2_Write((uint8_t)START_BYTE);
    UART2_Write(command_line);
    
    UART2_Write(bOBJ.TSL_gain);
    UART2_Write(bOBJ.TSL_integration_time);
    UART2_Write(bOBJ.DacCommand);
    UART2_Write((uint8_t)bOBJ.time_window);
    checksum = command_line + bOBJ.TSL_gain + bOBJ.TSL_integration_time + bOBJ.DacCommand + bOBJ.time_window;
    
    UART2_Write((uint8_t)(checksum >> 8));
    UART2_Write((uint8_t)(checksum & 0x00FF));  
    UART2_Write((uint8_t)STOP_BYTE);
    
}

bool bridgeValidateConfig(uint8_t* payload,uint8_t payload_size)
{
    if(payload_size == 4) // Hard coded value, if more parameters are needed, put theme in the loop and update the size
    {
        if(payload[0] < 4)
        {
            if(payload[1] < 6)
            {
                if(payload[3] < 11)
                {
                    bOBJ.TSL_gain = payload[0];
                    bOBJ.TSL_integration_time = payload[1];
                    bOBJ.DacCommand = payload[2];
                    bOBJ.time_window = payload[3];
                    return true;
                }
            }
        } 
    }
    return false;
    
}

void bridgeTimerElapsed(void)
{
    
    //printf("Timer elapsed\r\n");
    TMR1_StopTimer();
    BridgeState = eBridge_IDLE;
    
}

void bridgeDataRDY(void)
{
    DRDY = true;
}

void controlBridge_update(SMeasureParticlesObject* mOBJ)
{
    bOBJ.temperature = (uint16_t)(1000*mOBJ->m_fTemperatureCelcius);
    bOBJ.CH0_ON = mOBJ->m_uFullLighted;
    bOBJ.CH1_ON = mOBJ->m_uIrLighted;
    bOBJ.CH0_OFF = mOBJ->m_uFullDark;
    bOBJ.CH1_OFF = mOBJ->m_uIrDark;
    bOBJ.LED_current_meas = (uint16_t)(3.3*mOBJ->adcValue/4.096);
    bOBJ.time_window = mOBJ->m_uMeasureInterval;
    bOBJ.Lux_ON = (uint16_t)(1000*mOBJ->m_fLuxLighted);
    bOBJ.Lux_OFF = (uint16_t)(1000*mOBJ->m_fLuxDark);
    bOBJ.time_since_beginning = mOBJ->m_uLastRead;
    
    DRDY = false;
    
}

void updateCalculatedValues(void)
{
    static bool first_loop = true;
    static uint8_t mem_index = 0;
    static float mean = 0;
    float var = 0;
    float vtemp;
    
    if(!first_loop)
    {
        
        mean += ((float)bOBJ.CH0_ON - (float)bOBJ.CH0_BUFFER[mem_index])/(float)DATA_MEMORY_DEPTH;
        bOBJ.CH0_BUFFER[mem_index] = bOBJ.CH0_ON;
        bOBJ.slope = (int)(1000*(bOBJ.CH0_ON-mean)/mean);
        
        for(uint8_t i = 0;i < DATA_MEMORY_DEPTH;i++)
        {
            vtemp = ((float)bOBJ.CH0_BUFFER[i] - mean);
            vtemp = vtemp*vtemp/((float)DATA_MEMORY_DEPTH - 1);
            var += vtemp;
        }
        bOBJ.variance = (uint16_t)sqrt(var);
        
    }else
    {
        mean += (float)bOBJ.CH0_ON/(float)DATA_MEMORY_DEPTH;
        bOBJ.CH0_BUFFER[mem_index] = bOBJ.CH0_ON;
    }
    
    mem_index++;
    if(mem_index >= (uint8_t)DATA_MEMORY_DEPTH)
    {
        mem_index=0;
        first_loop = false;
    }
    
}

bool isBridgeRDY(void)
{
    return BridgeState == eBridge_PROCESSING_DATA;
}