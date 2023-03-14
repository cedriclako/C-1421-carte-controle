/*
Solutions Novika.
Copyright 2022
===========================================================================

Filename :      ControlBridge.c

Author(s):      Charles Richard, CPI # 6045522   

Public prefix : control

Project # : C-1421

Product: Mesure d'�missions polluantes

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
#include "ParameterFile.h"
#include "mcc_generated_files/uart2.h"
#include "mcc_generated_files/tmr1.h"
#include "TSL2591.h"
#include "ControlBridge.h"


#define MAX_BUFFER_SIZE 64

#define START_BYTE 0xCC
#define STOP_BYTE 0x99

#define WRITE_CMD 0xC0
#define FIRECNT_CMD 0x80
#define SETZERO_CMD 0x40
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
    
}EControlBridgeObject;

void bridgeTimerElapsed(void);
void updateCalculatedValues(void);

static EControlBridgeStates BridgeState;
static EControlBridgeObject bOBJ;
static bool DRDY = false;
static uint8_t TX_BUFFER[MAX_BUFFER_SIZE];
static bool set_zero_complete = false;

void sendData(uint8_t packet_size, uint8_t command);
void sendConfig(const gs_Parameters* Param);
uint8_t fillDataBuffer(const gs_MemoryParams* Param);
uint8_t fillEEBuffer(const gs_MemoryParams* Param);


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
    bOBJ.slope = 0;
    
    bOBJ.Lux_ON = 0;
    bOBJ.Lux_OFF = 0;
}

    
void ControlBridgeProcess(void)
{
    const gs_Parameters* CB_Param = PF_getCBParamAddr();
    const gs_MemoryParams* EE_Param = PF_getEEParamAddr();
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
                
                if((command & 0xC0) == READ_CMD || (command & 0xC0) == FIRECNT_CMD || (command & 0xC0) == SETZERO_CMD)
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
                        switch(command & 0xC0)
                        {
                            case READ_CMD:
                                BridgeState = eBridge_TRANSMIT_DATA;
                                break;
                            case FIRECNT_CMD:
                                PF_IncrementFireCount();
                                BridgeState = eBridge_IDLE;
                                break;
                            case SETZERO_CMD:
                                PF_requestReconfigure();
                                
                                
                                if(measureParticlesReadyForConfig())
                                {
                                    measureParticlesSetZero();
                                    PF_ToggleAcqEnable();
                                }
                                
                                if(set_zero_complete)
                                {
                                    BridgeState = eBridge_TRANSMIT_DATA;
                                    set_zero_complete = false;
                                }
                                
                                break;
                        }
                        
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
            switch(command & 0xC0)
            {
                case READ_CMD:
                    tx_size = fillDataBuffer(EE_Param);
                    sendData(tx_size, (uint8_t)READ_CMD);
                    break;
                case FIRECNT_CMD:
                case SETZERO_CMD:
                    tx_size = fillEEBuffer(EE_Param);
                    sendData(tx_size, (uint8_t)SETZERO_CMD);
                    break;
            }
            
            BridgeState = eBridge_IDLE;
            break;
        case eBridge_VERIFY_PAYLOAD:
            printf("Verifiyng payload\r\n");
            if(command == WRITE_CMD)
            {
                if(PF_validateConfig(payload, payload_size))
                {
                    BridgeState = eBridge_SET_CONFIG;
                    printf("Payload verified\r\n");
                    PF_requestReconfigure();
                }
            }else
            {
                BridgeState = eBridge_COMM_ERROR;
            }
            break;
        case eBridge_SET_CONFIG:
    
            if(measureParticlesReadyForConfig())
            {
                BridgeState = eBridge_TRANSMIT_CONFIG;
            }
            break;
        case eBridge_TRANSMIT_CONFIG:
            sendConfig(CB_Param);
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

uint8_t fillEEBuffer(const gs_MemoryParams* Param)
{
    TX_BUFFER[0] = (uint8_t)(Param->fireCounter >> 8);
    TX_BUFFER[1] = (uint8_t)(Param->fireCounter & 0x00FF);
    TX_BUFFER[2] = (uint8_t)(Param->lastZeroValue >> 8);
    TX_BUFFER[3] = (uint8_t)(Param->lastZeroValue & 0x00FF);
    TX_BUFFER[4] = Param->lastZeroDac;
    TX_BUFFER[5] = Param->lastZeroCurrent;
    TX_BUFFER[6] = Param->lastZeroTemp;
    return 7;
}

uint8_t fillDataBuffer(const gs_MemoryParams* Param)
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
    TX_BUFFER[24] = (uint8_t)(Param->lastZeroValue >> 8);
    TX_BUFFER[25] = (uint8_t)(Param->lastZeroValue & 0x00FF);
//    printf("%u, %u, %u, %u, %u, %u, %u, %i \r\n",bOBJ.CH0_ON, bOBJ.CH0_OFF, bOBJ.CH1_ON,bOBJ.CH1_OFF,bOBJ.variance,bOBJ.temperature,bOBJ.LED_current_meas,bOBJ.slope);
    //printf("%u, %u, %u, %i \r\n",bOBJ.CH0_ON,bOBJ.variance,bOBJ.LED_current_meas,bOBJ.slope);

    return 26;
}


void sendData(uint8_t packet_size, uint8_t command)
{
    uint16_t checksum;
    uint8_t command_line;
    
    command_line = (uint8_t) (command ^ packet_size);
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

void sendConfig(const gs_Parameters* Param)
{
    uint16_t checksum;
    uint8_t command_line;
    uint8_t packet_size = 4;
    
    command_line = (uint8_t) (WRITE_CMD ^ packet_size);

    UART2_Write((uint8_t)START_BYTE);
    UART2_Write(command_line);
    
    UART2_Write((uint8_t)Param->TSL_gain);
    UART2_Write((uint8_t)Param->TSL_integrationTime);
    UART2_Write(Param->DAC_value);
    UART2_Write((uint8_t)Param->MeasureInterval);
    checksum = command_line + Param->TSL_gain + Param->TSL_integrationTime + Param->DAC_value + Param->MeasureInterval;
    
    UART2_Write((uint8_t)(checksum >> 8));
    UART2_Write((uint8_t)(checksum & 0x00FF));  
    UART2_Write((uint8_t)STOP_BYTE);
    
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

void bridgeZeroComplete(void)
{
    set_zero_complete = true;
}

void controlBridge_update(SMeasureParticlesObject* mOBJ)
{
    bOBJ.temperature = (uint16_t)(1000*mOBJ->m_fTemperatureCelcius);
    bOBJ.CH0_ON = mOBJ->m_uFullLighted;
    bOBJ.CH1_ON = mOBJ->m_uIrLighted;
    bOBJ.CH0_OFF = mOBJ->m_uFullDark;
    bOBJ.CH1_OFF = mOBJ->m_uIrDark;
    if(bOBJ.LED_current_meas == 0)
    {
        bOBJ.LED_current_meas = (uint16_t)(0.33*mOBJ->adcValue/4.096);
    }else
    {
        bOBJ.LED_current_meas = 0.95*bOBJ.LED_current_meas + 0.05*(uint16_t)(0.33*mOBJ->adcValue/4.096);
    }
    
    bOBJ.Lux_ON = (uint16_t)(1000*mOBJ->m_fLuxLighted);
    bOBJ.Lux_OFF = (uint16_t)(1000*mOBJ->m_fLuxDark);
    bOBJ.time_since_beginning = mOBJ->m_uLastRead;
    //printf("DAC set to: %u\r\n", mOBJ->dacValue);
    
    DRDY = false;
    
}

void updateCalculatedValues(void)
{
    static bool first_loop = true;
    static uint8_t mem_index = 0;
    static float mean = 0;
    float var = 0;
    float vtemp;
    float alpha = 0.2;
    
    if(!first_loop)
    {
        
        mean += ((float)bOBJ.CH0_ON - (float)bOBJ.CH0_BUFFER[mem_index])/(float)DATA_MEMORY_DEPTH;
        bOBJ.CH0_BUFFER[mem_index] = bOBJ.CH0_ON;
        bOBJ.slope = (int)((1-alpha*bOBJ.slope) + (alpha*(1000*(bOBJ.CH0_ON-mean)/mean)));
        
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
    
    printf("#");	
    printf("PartCH0ON:%u ", bOBJ.CH0_ON);
    printf("PartCH1ON:%u ", bOBJ.CH1_ON);
    printf("PartCH0OFF:%u ",bOBJ.CH0_OFF);
    printf("PartCH1OFF:%u ",bOBJ.CH1_OFF);
    printf("PartVar:%u ",bOBJ.variance);
    printf("PartSlope:%i ",bOBJ.slope);
    printf("TPart:%u ",bOBJ.temperature);
    printf("PartCurr:%u ",bOBJ.LED_current_meas);
    printf("PartLuxON:%u ", bOBJ.Lux_ON);
    printf("PartLuxOFF:%u ", bOBJ.Lux_OFF);
    printf("PartTime:%lu ", bOBJ.time_since_beginning);
    printf("GlobalStatus:FORMAT_TBD" ); // Aller chercher le flag de particle adjust ou le temps de
    printf("*\n\r");
    
}

bool isBridgeRDY(void)
{
    return BridgeState == eBridge_PROCESSING_DATA;
}