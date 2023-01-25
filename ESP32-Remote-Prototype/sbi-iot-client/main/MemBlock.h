#ifndef _MEMBLOCK_H_
#define _MEMBLOCK_H_

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

typedef struct 
{
    bool has_sRemoteState;
    SBI_iot_RemoteState sRemoteState;

    bool has_s2cGetStatusResp;
    SBI_iot_S2CGetStatusResp s2cGetStatusResp;

    bool isTemperatureSetPointChanged;
    bool isFanSpeedSetPointChanged;
} MEMBLOCK_SMemBlock;

extern MEMBLOCK_SMemBlock g_sMemblock;

#ifdef __cplusplus
}
#endif

#endif