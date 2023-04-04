/*
Solutions Novika.
Copyright 2021
===========================================================================

Filename :      DebugInterface.c

Author(s):      Clement Soucy, CPI # 6027411
 *              Charles Richard, CPI # 6045522

Public prefix : Debug

Project # : C-1333, C-1421

Product: Mesure d'émissions polluantes

Creation date:  2021/06/21

Description:    Implementation debug console using uart1.

===========================================================================

Modifications
-------------

By    |   Date     | Version | Description
------+------------+---------+---------------------------------------------
CS    | 2021/06/21 | -       | Creation
CR    | 2022/10/14 | 0.1     | Corrections and addition of interactions
===========================================================================
*/
#include "ParameterFile.h"
#include "DebugInterface.h"
#include "MeasureParticles.h"

#include "mcc_generated_files/uart1.h"
#include "mcc_generated_files/dac1.h"
#include "mcc_generated_files/adcc.h"


#define SIZE_FIFO_TX   2000
#define SIZE_FIFO_RX   20

#define ASCII_CARRIAGE_RETURN 13

typedef struct
{
    uint8_t m_uRxFIFO[SIZE_FIFO_RX];
    uint16_t m_uHeadEnter;
    uint16_t m_uTailExit;
}SRxObject;

typedef struct
{
    uint8_t m_uTxFIFO[SIZE_FIFO_TX];
    uint16_t m_uHeadEnter;
    uint16_t m_uTailExit;
}STxObject;

static SRxObject gs_sRxObject;
static STxObject gs_sTxObject;

static adc_result_t gs_Curr;

void DebugPrintProjectInfo(void);
void DebugPrintMenu(void);
bool DebugIsRxFifoFull(void);
bool DebugRxStillAvailable(void);

void DebugReadUARTBuffer(void);
void DebugWriteUARTBuffer(void);

void DebugInitialize(void)
{
    gs_sRxObject.m_uHeadEnter = 0;
    gs_sRxObject.m_uTailExit = 0;
    
    gs_sTxObject.m_uHeadEnter = 0;
    gs_sTxObject.m_uTailExit = 0;
    
    gs_Curr = 0;
    
    DebugPrintProjectInfo();
    DebugPrintMenu();
}

void DebugProcess(void)
{
    const gs_Parameters* dbgParam = PF_getCBParamAddr();
    uint8_t uKey;
    static uint8_t uKeyFunct = 0;
    static uint32_t uKeyNum = 0;
    static uint32_t uFirstKeyNum = 0;
    // Load incoming bytes in FIFO
    while (UART1_is_rx_ready() && !DebugIsRxFifoFull())
    {
        // UART has new Rx bytes and FIFO is not full
        DebugReadUARTBuffer();
    }
    
    // Write next pending byte when UART is ready
    if (UART1_is_tx_ready())
    {
        // UART is ready to receive at least one byte
        DebugWriteUARTBuffer();
    }
    
    while(DebugRxStillAvailable())
    {
        uKey = getch();
        putch(uKey);
        
        // case insensitive
        if (uKey > 'Z')
        {
            uKey -= ('a' - 'A');
        }
            
        // Decode number
        if ((uKey >= '0') && (uKey <= '9'))
        {
            uKeyNum = uKeyNum * 10 + (uKey - '0');
        } 
        
        // If a space occur, save uKeyNum has being the leading number in the command
        if (uKey == ' ')
        {
            uFirstKeyNum = uKeyNum;
            uKeyNum = 0;
        }

        // Log the function character
        if ((uKey >= 'A') && (uKey <= 'Z'))
        {
            uKeyFunct = uKey;
        }
        
        if (uKey == ASCII_CARRIAGE_RETURN)
        {
            switch (uKeyFunct)
            {             
                case 'H':
                {
                    // printf("H --> Print this help menu\r\n");
                    
                    DebugPrintMenu();
                    break;
                }
                case 'P':
                {
                    printf("DAC set to: %u\r\n", dbgParam->DAC_value);
                    break;
                }
                case'Z':
                {
                    measureParticlesSetZero();
                    break;
                }
                case'S':
                {
                    PF_ToggleAcqEnable();
                    break;
                }
                case 'L':
                {
                    if(DAC1CONbits.DAC1EN)
                    {
                        DAC1_Disable();
                        printf("DAC DISABLED\r\n");
                    }
                    else
                    {
                        DAC1_Enable();
                        printf("DAC ENABLED\r\n");
                    }
                    
                    break;
                }
                case 'V':
                {
                    gs_Curr = ADCC_GetSingleConversion(channel_ANB7);
                    
                    printf("ADC VALUE = %.1f mV \r\n",(3300*(float)gs_Curr/4096));
                    printf("LED current = %.1f mA \r\n", (330*(float)gs_Curr/4096));
                    break;
                }
                default:
                {
                    break;
                }
            }
            
            // Reset decoded numbers for next cycle
            uKeyNum = 0;
            uFirstKeyNum = 0;
        }
    }
}


void DebugPrintProjectInfo(void)
{
    printf("\nC-1421 - Real-time particles monitoring - V 0.1\r\n");
}

void DebugPrintMenu(void)
{
    printf("\nH --> Print this help menu\r\n"); 
    printf("P --> Print DAC value [0:255] \r\n");
    printf("L --> Manually toggle LED\r\n"); 
    printf("S --> Start/Pause acquisition\r\n");
    printf("Z --> Set lux zero \r\n");
    printf("V --> Print ADC value and LED current\r\n");
      
}

void DebugReadUARTBuffer(void)
{
    if(((gs_sRxObject.m_uHeadEnter + 1) % SIZE_FIFO_RX) != gs_sRxObject.m_uTailExit)
    {
        // Space still available in FIFO
        gs_sRxObject.m_uRxFIFO[gs_sRxObject.m_uHeadEnter] = UART1_Read();
        gs_sRxObject.m_uHeadEnter ++;
        gs_sRxObject.m_uHeadEnter %= SIZE_FIFO_RX;
    }
}

void DebugWriteUARTBuffer(void)
{
    if (gs_sTxObject.m_uTailExit != gs_sTxObject.m_uHeadEnter)
    {
        // Data remaining to transmit
        UART1_Write(gs_sTxObject.m_uTxFIFO[gs_sTxObject.m_uTailExit]);
        gs_sTxObject.m_uTailExit ++;
        gs_sTxObject.m_uTailExit %= SIZE_FIFO_TX;
    }
}

void putch(char txData)
{
    // called by STOUT
    if ((gs_sTxObject.m_uHeadEnter == gs_sTxObject.m_uTailExit) && UART1_is_tx_ready())
    {
        // FIFO buffer was empty and space is availbale in UART, send it directly
        UART1_Write(txData);
    }
    else
    {
        if (((gs_sTxObject.m_uHeadEnter + 1) % SIZE_FIFO_TX) != gs_sTxObject.m_uTailExit)
        {
            // FIFO is not full
            gs_sTxObject.m_uTxFIFO[gs_sTxObject.m_uHeadEnter] = txData;
            gs_sTxObject.m_uHeadEnter ++;
            gs_sTxObject.m_uHeadEnter %= SIZE_FIFO_TX;
        }
    }
}

char getch(void)
{
    char cCharRet;
    
    if (gs_sRxObject.m_uTailExit != gs_sRxObject.m_uHeadEnter)
    {
        // Data has not been read yet
        cCharRet = (char) gs_sRxObject.m_uRxFIFO[gs_sRxObject.m_uTailExit];
        gs_sRxObject.m_uTailExit ++;
        gs_sRxObject.m_uTailExit %= SIZE_FIFO_RX;
    }
    else
    {
        cCharRet = 0;
    }
    
    return cCharRet;
}

bool DebugIsRxFifoFull(void)
{
    bool bFull = true;
    if(((gs_sRxObject.m_uHeadEnter + 1) % SIZE_FIFO_RX) != gs_sRxObject.m_uTailExit)
    {
        bFull = false;
    }
    
    return bFull;
}

bool DebugRxStillAvailable(void)
{
    bool bAvailable = true;
    
    if (gs_sRxObject.m_uTailExit == gs_sRxObject.m_uHeadEnter)
    {
        bAvailable = false;
    }
    
    return bAvailable;
}
