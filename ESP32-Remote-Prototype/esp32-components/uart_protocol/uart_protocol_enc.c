#include <assert.h>
#include "uart_protocol_enc.h"

void UARTPROTOCOLENC_Init(UARTPROTOCOLENC_SHandle* psHandle, const UARTPROTOCOLENC_SConfig* psConfig)
{
    assert(psHandle != NULL && psConfig != NULL);
    assert(psConfig->fnWriteCb != NULL);

    psHandle->psConfig = psConfig;
}

bool UARTPROTOCOLENC_Send(UARTPROTOCOLENC_SHandle* psHandle, uint8_t u8ID, const uint8_t u8Payloads[], uint16_t u16PayloadLen)
{
    if (u16PayloadLen > UARTPROTOCOLCOMMON_MAXPAYLOAD)
        return false;

    uint8_t u8Checksum = 0;
    const uint8_t u8STARTBYTE = UARTPROTOCOLCOMMON_START_BYTE;
    psHandle->psConfig->fnWriteCb(psHandle, &u8STARTBYTE, 1);
    psHandle->psConfig->fnWriteCb(psHandle, &u8ID, 1);
    u8Checksum += u8ID;
    // Payload in LITTLE ENDIAN format
    const uint8_t u8LenB0 = (uint8_t)(u16PayloadLen & 0xFF);
    psHandle->psConfig->fnWriteCb(psHandle, &u8LenB0, 1);
    const uint8_t u8LenB1 = (uint8_t)((u16PayloadLen >> 8) & 0xFF);
    psHandle->psConfig->fnWriteCb(psHandle, &u8LenB1, 1);
    // Calculate checksum on the fly ...
    if (u8Payloads != NULL && u16PayloadLen > 0)
    {
        u8Checksum += u8LenB0;
        u8Checksum += u8LenB1;
        for(int i = 0; i < u16PayloadLen; i++)
            u8Checksum += u8Payloads[i];
        psHandle->psConfig->fnWriteCb(psHandle, u8Payloads, u16PayloadLen);
    }
    u8Checksum = ~u8Checksum;

    psHandle->psConfig->fnWriteCb(psHandle, &u8Checksum, 1);
    const uint8_t u8STOPBYTE = UARTPROTOCOLCOMMON_STOP_BYTE;
    psHandle->psConfig->fnWriteCb(psHandle, &u8STOPBYTE, 1);
    return true;
}

