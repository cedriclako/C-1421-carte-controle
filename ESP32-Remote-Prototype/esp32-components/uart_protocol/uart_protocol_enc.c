#include <assert.h>
#include "uart_protocol_enc.h"

void UARTPROTOCOLENC_Init(UARTPROTOCOLENC_SHandle* psHandle, const UARTPROTOCOLENC_SConfig* psConfig)
{
    assert(psHandle != NULL && psConfig != NULL);
    assert(psHandle->psConfig->fnWriteCb != NULL);
}

void UARTPROTOCOLENC_Send(UARTPROTOCOLENC_SHandle* psHandle, uint8_t u8ID, const uint8_t u8Payloads[], uint8_t u8PayloadLen)
{
    const uint8_t u8STARTBYTE = UARTPROTOCOLCOMMON_START_BYTE;
    psHandle->psConfig->fnWriteCb(psHandle, &u8STARTBYTE, 1);
    psHandle->psConfig->fnWriteCb(psHandle, &u8ID, 1);
    const uint8_t u8Len = u8PayloadLen;
    psHandle->psConfig->fnWriteCb(psHandle, &u8Len, 1);
    psHandle->psConfig->fnWriteCb(psHandle, u8Payloads, u8PayloadLen);

    uint8_t u8Checksum = 0;
    for(int i = 0; i < u8PayloadLen; i++)
        u8Checksum += u8Payloads[i];
    u8Checksum = ~u8Checksum;

    psHandle->psConfig->fnWriteCb(psHandle, &u8Checksum, 1);
    const uint8_t u8STOPBYTE = UARTPROTOCOLCOMMON_STOP_BYTE;
    psHandle->psConfig->fnWriteCb(psHandle, &u8STOPBYTE, 1);
}

