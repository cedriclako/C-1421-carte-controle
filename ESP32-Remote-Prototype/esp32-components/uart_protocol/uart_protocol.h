#ifndef _UART_PROTOCOL_H_
#define _UART_PROTOCOL_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define UARTPROTOCOL_MAXPAYLOAD 255
#define UARTPROTOCOL_START_BYTE 0xCC
#define UARTPROTOCOL_STOP_BYTE 0x99

// Looks like this: [START BYTE] [Frame ID] [PAYLOAD LENGTH] [PAYLOAD  ...] [CHECKSUM] [STOP BYTE] 
typedef enum
{
  UARTPROTOCOL_ESTEP_WaitingForStartByte,  
  UARTPROTOCOL_ESTEP_WaitingFrameID,  
  UARTPROTOCOL_ESTEP_WaitingPayloadLength,
  UARTPROTOCOL_ESTEP_GettingPayload,
  UARTPROTOCOL_ESTEP_WaitingChecksum,
  UARTPROTOCOL_ESTEP_WaitingStopByte
} UARTPROTOCOL_ESTEP;

// Allows to use handle inside callback.
typedef struct _UARTPROTOCOL_SHandle UARTPROTOCOL_SHandle;

// ------------------------------------
// Callbacks
// ------------------------------------
typedef void (*FnAcceptFrame)(const UARTPROTOCOL_SHandle* psHandle, const uint8_t u8Payloads[], uint8_t u8PayloadLen);
typedef void (*FnDropFrame)(const UARTPROTOCOL_SHandle* psHandle, const char* szReason);
typedef int64_t (*FnGetTimerCountMS)(const UARTPROTOCOL_SHandle* psHandle);

typedef struct 
{
    uint8_t* u8PayloadBuffers;
    uint8_t u8PayloadBufferLen;

    uint32_t u32FrameReceiveTimeOutMS;

    // Callback configs
    const FnAcceptFrame fnAcceptFrameCb;
    const FnDropFrame fnDropFrameCb;
    const FnGetTimerCountMS fnGetTimerCountMSCb;
} UARTPROTOCOL_SConfig;

struct _UARTPROTOCOL_SHandle
{
    // Current step
    UARTPROTOCOL_ESTEP eStep;
    uint32_t u32PayloadCount;

    int64_t s64StartTimeMS;

    // Frame payload len. 
    uint8_t u8FrameID;
    uint8_t u8FramePayloadLen;

    // On the fly checksum calculation
    uint8_t u8ChecksumCalculation;

    const UARTPROTOCOL_SConfig* psConfig;
};

void UARTPROTOCOL_Init(UARTPROTOCOL_SHandle* psHandle, const UARTPROTOCOL_SConfig* psConfig);

void UARTPROTOCOL_Reset(UARTPROTOCOL_SHandle* psHandle);

void UARTPROTOCOL_HandleIn(UARTPROTOCOL_SHandle* psHandle, const uint8_t* u8Datas, uint32_t u32DataLen);

#endif