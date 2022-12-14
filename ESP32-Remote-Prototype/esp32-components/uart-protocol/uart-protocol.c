#include <stdio.h>
#include <assert.h>
#include "uart-protocol.h"

static void AddByte(UARTPROTOCOL_SHandle* psHandle, uint8_t u8);

static void DropFrame(UARTPROTOCOL_SHandle* psHandle, const char* szReason);
static void AcceptFrame(UARTPROTOCOL_SHandle* psHandle);

void UARTPROTOCOL_Init(UARTPROTOCOL_SHandle* psHandle, const UARTPROTOCOL_SConfig* psConfig)
{
    assert(psHandle != NULL && psConfig != NULL && psConfig->u8PayloadBuffers != NULL);
    psHandle->psConfig = psConfig;
}

void UARTPROTOCOL_Reset(UARTPROTOCOL_SHandle* psHandle)
{
    psHandle->eStep = UARTPROTOCOL_ESTEP_WaitingForStartByte;
    psHandle->u32PayloadCount = 0;

    psHandle->s64StartTimeMS = 0;
    psHandle->u8FrameID = 0;

    psHandle->u8ChecksumCalculation = 0;
}

void UARTPROTOCOL_HandleIn(UARTPROTOCOL_SHandle* psHandle, const uint8_t* u8Datas, uint32_t u32DataLen)
{
    for(uint32_t i = 0; i < u32DataLen; i++)
        AddByte(psHandle, u8Datas[i]);
}

static void AddByte(UARTPROTOCOL_SHandle* psHandle, uint8_t u8)
{
    // Timeout is supported but optional ...
    if (psHandle->eStep != UARTPROTOCOL_ESTEP_WaitingForStartByte)
    {
        if (psHandle->psConfig->fnGetTimerCountMSCb != NULL &&
            psHandle->psConfig->u32FrameReceiveTimeOutMS > 0)
        {       
            const int64_t s64TimeDiffMS = psHandle->psConfig->fnGetTimerCountMSCb() - psHandle->s64StartTimeMS;
            if (s64TimeDiffMS > psHandle->psConfig->u32FrameReceiveTimeOutMS)
            {
                // We don't break here on purpose, we give it a chance to start a new frame.
                DropFrame(psHandle, "Timeout");
            }
        }
    }

    switch(psHandle->eStep)
    {
        case UARTPROTOCOL_ESTEP_WaitingForStartByte:
        {
            // Wait until we get a start byte ...
            if (u8 == UARTPROTOCOL_START_BYTE)
            {
                psHandle->u8ChecksumCalculation = 0;

                // IF we support timeout ...
                if (psHandle->psConfig->fnGetTimerCountMSCb != NULL)
                    psHandle->s64StartTimeMS = psHandle->psConfig->fnGetTimerCountMSCb();
                else
                    psHandle->s64StartTimeMS = 0;

                psHandle->eStep = UARTPROTOCOL_ESTEP_WaitingFrameID;
            }
            break;
        }
        case UARTPROTOCOL_ESTEP_WaitingFrameID:
        {
            psHandle->u8FrameID = u8;
            psHandle->eStep = UARTPROTOCOL_ESTEP_WaitingPayloadLength;
            psHandle->u8ChecksumCalculation += u8;
            break;
        }
        case UARTPROTOCOL_ESTEP_WaitingPayloadLength:
        {
            psHandle->u8FramePayloadLen = u8;
            if (psHandle->u8FramePayloadLen > psHandle->psConfig->u8PayloadBufferLen)
            {
                DropFrame(psHandle, "Payload is too big for the buffer");
                break;
            }

            psHandle->u8ChecksumCalculation += u8;
            psHandle->eStep = UARTPROTOCOL_ESTEP_GettingPayload;
            break;
        }
        case UARTPROTOCOL_ESTEP_GettingPayload:
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
                psHandle->eStep = UARTPROTOCOL_ESTEP_WaitingChecksum;
            break;
        }
        case UARTPROTOCOL_ESTEP_WaitingChecksum:
        {
            // Bitwise operation on calculated checksum ...
            // Checksum arrived ...
            if (~psHandle->u8ChecksumCalculation != u8)
            {
                DropFrame(psHandle, "Invalid checksum");
                break;
            }
            
            psHandle->eStep = UARTPROTOCOL_ESTEP_WaitingStopByte;
            break;
        }
        case UARTPROTOCOL_ESTEP_WaitingStopByte:
        {
            // If we reach this point, the checksum passed
            if (u8 != UARTPROTOCOL_STOP_BYTE)
            {
                DropFrame(psHandle, "Stop byte");
                break;
            }

            // If we reach this point it's good.
            AcceptFrame(psHandle);
            UARTPROTOCOL_Reset(psHandle);
            break;
        }
    }
}

static void AcceptFrame(UARTPROTOCOL_SHandle* psHandle)
{
    if (psHandle->psConfig->fnAcceptFrameCb != NULL)
        psHandle->psConfig->fnAcceptFrameCb(psHandle, psHandle->psConfig->u8PayloadBuffers, (uint8_t)psHandle->u32PayloadCount);
    UARTPROTOCOL_Reset(psHandle);
}

static void DropFrame(UARTPROTOCOL_SHandle* psHandle, const char* szReason)
{
    if (psHandle->psConfig->fnDropFrameCb != NULL)
        psHandle->psConfig->fnDropFrameCb(psHandle, szReason);
    UARTPROTOCOL_Reset(psHandle);
}
