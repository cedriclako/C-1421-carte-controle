#ifndef _STOVEMB_H_
#define _STOVEMB_H_

#include <stdint.h>
#include <stdbool.h>
#include "ufec23_protocol.h"
#include "ufec23_endec.h"

#include "SBI.iot.pb.h"
#include "SBI.iot.common.pb.h"
#include "SBI-iot-util.h"

#define STOVEMB_MAXIMUMSETTING_ENTRIES (150)

typedef struct
{
    UFEC23ENDEC_SEntry sEntry;

    // Value to write
    UFEC23ENDEC_uValue sWriteValue;
    bool bIsNeedWrite;
} STOVEMB_SParameterEntry;

typedef struct 
{
    // Temperature setpoint
    bool has_temp_sp;
    SBI_iot_common_TemperatureSetPoint temp_sp;
    
    // Last current (C)
    bool has_tempC_current;
    float tempC_current;

    // Fan
    bool hasFanSpeed;
    bool isFanSpeedAutomatic;
    uint8_t u8FanSpeedCurr;

    // Last communication ticks
    TickType_t ttLastCommunicationTicks; 
} STOVEMB_SRemoteData;

typedef struct 
{
    // Settings
    UFEC23ENDEC_S2CGetRunningSettingResp s2CGetRunningSetting;
    bool s2CGetRunningSettingIsSet;

    // Version infos
    UFEC23ENDEC_S2CReqVersionResp sS2CReqVersionResp;
    bool sS2CReqVersionRespIsSet;

    // Remote related data
    STOVEMB_SRemoteData sRemoteData;

    // Parameter JSON
    volatile bool bIsParameterDownloadCompleted;
    STOVEMB_SParameterEntry arrParameterEntries[STOVEMB_MAXIMUMSETTING_ENTRIES]; // 100 maximum for now
    uint32_t u32ParameterCount;

    bool bIsAnyUploadError;
    bool bIsAnyDownloadError;

    // Is stove connected
    bool bIsStoveConnectedAndReady;

} STOVEMB_SMemBlock;

void STOVEMB_Init();

bool STOVEMB_Take(TickType_t xTicksToWait);

void STOVEMB_Give();

STOVEMB_SMemBlock* STOVEMB_GetMemBlock();

const STOVEMB_SMemBlock* STOVEMB_GetMemBlockRO();

int32_t STOVEMB_FindNextWritable(int32_t s32IndexStart, STOVEMB_SParameterEntry* pEntry);

STOVEMB_SParameterEntry* STOVEMB_GetByIndex(int32_t s32Index);

void STOVEMB_ResetAllParameterWriteFlag();

char* STOVEMB_ExportParamToJSON();

bool STOVEMB_InputParamFromJSON(const char* szJSON, char* szDstError, uint32_t u32DstErrorLen);

#endif