#ifndef _STOVEMB_H_
#define _STOVEMB_H_

#include <stdint.h>
#include <stdbool.h>
#include "ufec23_protocol.h"
#include "ufec23_endec.h"

#define STOVEMB_MAXIMUMSETTING_ENTRIES (100)

typedef struct 
{
    UFEC23ENDEC_S2CGetRunningSettingResp s2CGetRunningSetting;
    bool s2CGetRunningSettingIsSet;

    UFEC23ENDEC_S2CReqVersionResp sS2CReqVersionResp;
    bool sS2CReqVersionRespIsSet;

    // Parameter JSON
    volatile bool bIsParameterDownloadCompleted;
    UFEC23ENDEC_SEntry arrParameterEntries[STOVEMB_MAXIMUMSETTING_ENTRIES]; // 100 maximum for now
    uint32_t u32ParameterCount;
} STOVEMB_SMemBlock;

void STOVEMB_Init();

bool STOVEMB_Take(TickType_t xTicksToWait);

void STOVEMB_Give();

STOVEMB_SMemBlock* STOVEMB_GetMemBlock();

const STOVEMB_SMemBlock* STOVEMB_GetMemBlockRO();

char* STOVEMB_ExportParamToJSON();

#endif