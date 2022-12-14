#ifndef _UART_PROTOCOLDEC_H_
#define _UART_PROTOCOLDEC_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "uart_protocol_common.h"

// Looks like this: [START BYTE] [Frame ID] [PAYLOAD LENGTH] [PAYLOAD  ...] [CHECKSUM] [STOP BYTE] 
typedef enum
{
  UARTPROTOCOLDEC_ESTEP_WaitingForStartByte,  
  UARTPROTOCOLDEC_ESTEP_WaitingFrameID,  
  UARTPROTOCOLDEC_ESTEP_WaitingPayloadLength,
  UARTPROTOCOLDEC_ESTEP_GettingPayload,
  UARTPROTOCOLDEC_ESTEP_WaitingChecksum,
  UARTPROTOCOLDEC_ESTEP_WaitingStopByte
} UARTPROTOCOLDEC_ESTEP;

// Allows to use handle inside callback.
typedef struct _UARTPROTOCOLDEC_SHandle UARTPROTOCOLDEC_SHandle;

// ------------------------------------
// Callbacks
// ------------------------------------
typedef void (*FnAcceptFrame)(const UARTPROTOCOLDEC_SHandle* psHandle, uint8_t u8ID, const uint8_t u8Payloads[], uint8_t u8PayloadLen);
typedef void (*FnDropFrame)(const UARTPROTOCOLDEC_SHandle* psHandle, const char* szReason);
typedef int64_t (*FnGetTimerCountMS)(const UARTPROTOCOLDEC_SHandle* psHandle);

typedef struct 
{
    uint8_t* u8PayloadBuffers;
    uint8_t u8PayloadBufferLen;

    uint32_t u32FrameReceiveTimeOutMS;

    // Callback configs
    const FnAcceptFrame fnAcceptFrameCb;
    const FnDropFrame fnDropFrameCb;
    const FnGetTimerCountMS fnGetTimerCountMSCb;
} UARTPROTOCOLDEC_SConfig;

struct _UARTPROTOCOLDEC_SHandle
{
    // Current step
    UARTPROTOCOLDEC_ESTEP eStep;
    uint32_t u32PayloadCount;

    int64_t s64StartTimeMS;

    // Frame payload len. 
    uint8_t u8FrameID;
    uint8_t u8FramePayloadLen;

    // On the fly checksum calculation
    uint8_t u8ChecksumCalculation;

    const UARTPROTOCOLDEC_SConfig* psConfig;
};

void UARTPROTOCOLDEC_Init(UARTPROTOCOLDEC_SHandle* psHandle, const UARTPROTOCOLDEC_SConfig* psConfig);

void UARTPROTOCOLDEC_Reset(UARTPROTOCOLDEC_SHandle* psHandle);

void UARTPROTOCOLDEC_HandleIn(UARTPROTOCOLDEC_SHandle* psHandle, const uint8_t* u8Datas, uint32_t u32DataLen);

#endif