#ifndef _UART_PROTOCOLENC_H_
#define _UART_PROTOCOLENC_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "uart_protocol_common.h"

// Allows to use handle inside callback.
typedef struct _UARTPROTOCOLENC_SHandle UARTPROTOCOLENC_SHandle;

typedef void (*FnWrite)(const UARTPROTOCOLENC_SHandle* psHandle, const uint8_t u8Datas[], uint32_t u32DataLen);

typedef struct 
{
    FnWrite fnWriteCb;
} UARTPROTOCOLENC_SConfig;

struct _UARTPROTOCOLENC_SHandle
{
    const UARTPROTOCOLENC_SConfig* psConfig;
};

void UARTPROTOCOLENC_Init(UARTPROTOCOLENC_SHandle* psHandle, const UARTPROTOCOLENC_SConfig* psConfig);

bool UARTPROTOCOLENC_Send(UARTPROTOCOLENC_SHandle* psHandle, uint8_t u8ID, const uint8_t u8Payloads[], uint32_t u32PayloadLen);

#endif
