#ifndef _UART_PROTOCOL_H_
#define _UART_PROTOCOL_H_

#include <stddef.h>
#include <stdint.h>

#define UARTPROTOCOL_MAXPAYLOAD 255
#define UARTPROTOCOL_START_BYTE 0xCC
#define UARTPROTOCOL_STOP_BYTE 0x99

typedef enum
{
  UARTPROTOCOL_ESTEP_WaitingForStartByte,  
} UARTPROTOCOL_ESTEP;

typedef struct 
{
    UARTPROTOCOL_ESTEP eStep;
} UARTPROTOCOL_SHandle;

#endif