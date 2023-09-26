#include <assert.h>
#include "uart_protocol_enc.h"

void UARTPROTOCOLENC_Init(UARTPROTOCOLENC_SHandle* psHandle, const UARTPROTOCOLENC_SConfig* psConfig)
{
    assert(psHandle != NULL && psConfig != NULL);
    assert(psConfig->fnWriteCb != NULL);

    psHandle->psConfig = psConfig;
}

bool UARTPROTOCOLENC_Send(UARTPROTOCOLENC_SHandle* psHandle, uint8_t u8ID, const uint8_t u8Payloads[], uint32_t u32PayloadLen)
{
    if (u32PayloadLen > UARTPROTOCOLCOMMON_MAXPAYLOAD)
        return false;

    uint8_t u8Checksum = 0;
    uint8_t u8TxBuffer[UARTPROTOCOLCOMMON_MAXPAYLOAD+6] = {0x00};

    u8TxBuffer[0] = (uint8_t)UARTPROTOCOLCOMMON_START_BYTE;
    u8TxBuffer[1] = u8ID;
    // Payload in LITTLE ENDIAN format
    u8TxBuffer[2] = (uint8_t)(u32PayloadLen & 0xFF);
    u8TxBuffer[3] = (uint8_t)((u32PayloadLen >> 8) & 0xFF);

    if (u8Payloads != NULL && u32PayloadLen > 0)
    {
    	for(uint16_t i = 0; i < u32PayloadLen; i++)
    	{
    		u8TxBuffer[i+4] = u8Payloads[i];
    	}
    }

    for(uint16_t i = 1; i < u32PayloadLen + 4; i++)
    {
    	u8Checksum += (uint8_t)u8TxBuffer[i];
    }

    u8TxBuffer[u32PayloadLen + 4] = (uint8_t)(~u8Checksum);
    u8TxBuffer[u32PayloadLen + 5] = (uint8_t) UARTPROTOCOLCOMMON_STOP_BYTE;

    psHandle->psConfig->fnWriteCb(psHandle, u8TxBuffer, (uint32_t)(u32PayloadLen + 6));
    return true;
}

