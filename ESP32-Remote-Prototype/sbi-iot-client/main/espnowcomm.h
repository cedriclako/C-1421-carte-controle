#ifndef _ESPNOWCOMM_H_
#define _ESPNOWCOMM_H_

#include "SBI.iot.BaseProtocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "SBI.iot.BaseProtocol.h"
#include "SBI.iot.pb.h"
#include "SBI.iot.common.pb.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESPNOWCOMM_QUEUERX (5)

#define ESPNOWCOMM_SCANCHANNEL_TIMEOUTPERCHAN_MS (50)
#define ESPNOWCOMM_GETSTATUS_RETRY_MS (100)

typedef struct
{
    uint8_t u8Buffers[SBIIOTBASEPROTOCOL_MAXPAYLOADLEN];
    uint8_t u8BufferCount;
} ESPNOWCOMM_SMsg;

typedef void (*fnChannelFound)(uint8_t u8Channel);
typedef void (*fnS2CGetStatusResp)(const SBI_iot_S2CGetStatusResp* pMsg);

void ESPNOWCOMM_Init(uint8_t u8CurrChannel);

void ESPNOWCOMM_Handler();

void ESPNOWCOMM_SetChannelFoundCallback(fnChannelFound fnChannelFoundCb);

void ESPNOWCOMM_SetS2CGetStatusRespCallback(fnS2CGetStatusResp fnS2CGetStatusRespCb);

void ESPNOWCOMM_SendChangeSetting();

#ifdef __cplusplus
}
#endif

#endif