#include <stdio.h>
#include <assert.h>
#include <inttypes.h>
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
    psHandle->u8CurrentFrameID = 0;

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
                psHandle->u32CurrentFramePayloadLen = 0;
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
            psHandle->u8ChecksumCalculation += u8;
            psHandle->u8CurrentFrameID = u8;
            psHandle->eStep = UARTPROTOCOLDEC_ESTEP_WaitingPayloadLengthB0;
            break;
        }
        case UARTPROTOCOLDEC_ESTEP_WaitingPayloadLengthB0:
        {
            psHandle->u8ChecksumCalculation += u8;
            psHandle->u32CurrentFramePayloadLen = u8;

            psHandle->eStep = UARTPROTOCOLDEC_ESTEP_WaitingPayloadLengthB1;
            break;
        }
        case UARTPROTOCOLDEC_ESTEP_WaitingPayloadLengthB1:
        {
            psHandle->u8ChecksumCalculation += u8;
            // Little endian ...
            psHandle->u32CurrentFramePayloadLen |= (uint16_t)((uint16_t)u8 << 8);

            if (psHandle->u32CurrentFramePayloadLen > UARTPROTOCOLCOMMON_MAXPAYLOAD)
            {
                DropFrame(psHandle, "Payload is too big for the protocol");
                break;
            }
            
            if (psHandle->u32CurrentFramePayloadLen > psHandle->psConfig->u32PayloadBufferLen)
            {
                DropFrame(psHandle, "Payload is too big for the buffer");
                break;
            }

            // 0 byte payload are supported
            if (psHandle->u32CurrentFramePayloadLen == 0)
            	psHandle->eStep = UARTPROTOCOLDEC_ESTEP_WaitingChecksum;
            else
            	psHandle->eStep = UARTPROTOCOLDEC_ESTEP_GettingPayload;
            break;
        }
        case UARTPROTOCOLDEC_ESTEP_GettingPayload:
        {
            psHandle->psConfig->u8PayloadBuffers[psHandle->u32PayloadCount] = u8;
            psHandle->u8ChecksumCalculation += u8;
            psHandle->u32PayloadCount++;

            // Complete payload detected ...
            if (psHandle->u32PayloadCount >= (uint32_t)psHandle->u32CurrentFramePayloadLen)
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
                char tmp[128+1];
                snprintf(tmp, sizeof(tmp), "Invalid checksum, frameID: %"PRId32", expected: %2"PRIx32", got: %2"PRIx32", len: %"PRIx32, 
                    (int32_t)psHandle->u8CurrentFrameID,
                    (int32_t)u8CurrChecksum, (int32_t)u8, (int32_t)psHandle->u32CurrentFramePayloadLen);
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
                DropFrame(psHandle, "Not a stop byte");
                break;
            }

            // If we reach this point it's good.
            AcceptFrame(psHandle);
            UARTPROTOCOLDEC_Reset(psHandle);
            break;
        }
        default:
        	break;
    }
}

static void AcceptFrame(UARTPROTOCOLDEC_SHandle* psHandle)
{
    if (psHandle->psConfig->fnAcceptFrameCb != NULL)
        psHandle->psConfig->fnAcceptFrameCb(psHandle, psHandle->u8CurrentFrameID, psHandle->psConfig->u8PayloadBuffers, psHandle->u32PayloadCount);
    UARTPROTOCOLDEC_Reset(psHandle);
}

static void DropFrame(UARTPROTOCOLDEC_SHandle* psHandle, const char* szReason)
{
    if (psHandle->psConfig->fnDropFrameCb != NULL)
        psHandle->psConfig->fnDropFrameCb(psHandle, szReason);
    UARTPROTOCOLDEC_Reset(psHandle);
}
