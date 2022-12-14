#include <stdio.h>
#include <assert.h>
#include "uart_protocol_dec.h"

static void AddByte(UARTPROTOCOLDEC_SHandle* psHandle, uint8_t u8);

static void DropFrame(UARTPROTOCOLDEC_SHandle* psHandle, const char* szReason);
static void AcceptFrame(UARTPROTOCOLDEC_SHandle* psHandle);

void UARTPROTOCOLDEC_Init(UARTPROTOCOLDEC_SHandle* psHandle, const UARTPROTOCOLDEC_SConfig* psConfig)
{
    assert(psHandle != NULL && psConfig != NULL && psConfig->u8PayloadBuffers != NULL);
    psHandle->psConfig = psConfig;
}

void UARTPROTOCOLDEC_Reset(UARTPROTOCOLDEC_SHandle* psHandle)
{
    psHandle->eStep = UARTPROTOCOLDEC_ESTEP_WaitingForStartByte;
    psHandle->u32PayloadCount = 0;

    psHandle->s64StartTimeMS = 0;
    psHandle->u8FrameID = 0;

    psHandle->u8ChecksumCalculation = 0;
}

void UARTPROTOCOLDEC_HandleIn(UARTPROTOCOLDEC_SHandle* psHandle, const uint8_t* u8Datas, uint32_t u32DataLen)
{
    for(uint32_t i = 0; i < u32DataLen; i++)
        AddByte(psHandle, u8Datas[i]);
}

static void AddByte(UARTPROTOCOLDEC_SHandle* psHandle, uint8_t u8)
{
    // Timeout is supported but optional ...
    if (psHandle->eStep != UARTPROTOCOLDEC_ESTEP_WaitingForStartByte)
    {
        if (psHandle->psConfig->fnGetTimerCountMSCb != NULL &&
            psHandle->psConfig->u32FrameReceiveTimeOutMS > 0)
        {       
            const int64_t s64TimeDiffMS = psHandle->psConfig->fnGetTimerCountMSCb(psHandle) - psHandle->s64StartTimeMS;
            if (s64TimeDiffMS > psHandle->psConfig->u32FrameReceiveTimeOutMS)
            {
                // We don't break here on purpose, we give it a chance to start a new frame.
                DropFrame(psHandle, "Timeout");
            }
        }
    }

    switch(psHandle->eStep)
    {
        case UARTPROTOCOLDEC_ESTEP_WaitingForStartByte:
        {
            // Wait until we get a start byte ...
            if (u8 == UARTPROTOCOLCOMMON_START_BYTE)
            {
                psHandle->u8ChecksumCalculation = 0;

                // IF we support timeout ...
                if (psHandle->psConfig->fnGetTimerCountMSCb != NULL)
                    psHandle->s64StartTimeMS = psHandle->psConfig->fnGetTimerCountMSCb(psHandle);
                else
                    psHandle->s64StartTimeMS = 0;

                psHandle->eStep = UARTPROTOCOLDEC_ESTEP_WaitingFrameID;
            }
            break;
        }
        case UARTPROTOCOLDEC_ESTEP_WaitingFrameID:
        {
            psHandle->u8FrameID = u8;
            psHandle->eStep = UARTPROTOCOLDEC_ESTEP_WaitingPayloadLength;
            psHandle->u8ChecksumCalculation += u8;
            break;
        }
        case UARTPROTOCOLDEC_ESTEP_WaitingPayloadLength:
        {
            psHandle->u8FramePayloadLen = u8;
            if (psHandle->u8FramePayloadLen > psHandle->psConfig->u8PayloadBufferLen)
            {
                DropFrame(psHandle, "Payload is too big for the buffer");
                break;
            }

            psHandle->u8ChecksumCalculation += u8;
            psHandle->eStep = UARTPROTOCOLDEC_ESTEP_GettingPayload;
            break;
        }
        case UARTPROTOCOLDEC_ESTEP_GettingPayload:
        {
            if (psHandle->u32PayloadCount + 1 >= (uint32_t)psHandle->psConfig->u8PayloadBufferLen)
            {
                DropFrame(psHandle, "Payload is too big for the buffer");
                break;
            }

            psHandle->psConfig->u8PayloadBuffers[psHandle->u32PayloadCount] = u8;
            psHandle->u8ChecksumCalculation += u8;
            psHandle->u32PayloadCount++;

            // Complete payload detected ...
            if (psHandle->u32PayloadCount >= psHandle->u8FramePayloadLen)
                psHandle->eStep = UARTPROTOCOLDEC_ESTEP_WaitingChecksum;
            break;
        }
        case UARTPROTOCOLDEC_ESTEP_WaitingChecksum:
        {
            // Bitwise operation on calculated checksum ...
            // Checksum arrived ...
            const uint8_t u8CurrChecksum = ~psHandle->u8ChecksumCalculation;
            if (u8CurrChecksum != u8)
            {
                char tmp[64+1];
                snprintf(tmp, sizeof(tmp), "Invalid checksum, expected: %2X, got: %2X", u8CurrChecksum, u8);
                DropFrame(psHandle, tmp);
                break;
            }
            
            psHandle->eStep = UARTPROTOCOLDEC_ESTEP_WaitingStopByte;
            break;
        }
        case UARTPROTOCOLDEC_ESTEP_WaitingStopByte:
        {
            // If we reach this point, the checksum passed
            if (u8 != UARTPROTOCOLCOMMON_STOP_BYTE)
            {
                DropFrame(psHandle, "Stop byte");
                break;
            }

            // If we reach this point it's good.
            AcceptFrame(psHandle);
            UARTPROTOCOLDEC_Reset(psHandle);
            break;
        }
    }
}

static void AcceptFrame(UARTPROTOCOLDEC_SHandle* psHandle)
{
    if (psHandle->psConfig->fnAcceptFrameCb != NULL)
        psHandle->psConfig->fnAcceptFrameCb(psHandle, psHandle->u8FrameID, psHandle->psConfig->u8PayloadBuffers, (uint8_t)psHandle->u32PayloadCount);
    UARTPROTOCOLDEC_Reset(psHandle);
}

static void DropFrame(UARTPROTOCOLDEC_SHandle* psHandle, const char* szReason)
{
    if (psHandle->psConfig->fnDropFrameCb != NULL)
        psHandle->psConfig->fnDropFrameCb(psHandle, szReason);
    UARTPROTOCOLDEC_Reset(psHandle);
}
