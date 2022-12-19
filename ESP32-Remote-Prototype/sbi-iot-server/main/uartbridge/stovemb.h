#ifndef _STOVEMB_H_
#define _STOVEMB_H_

#include <stdint.h>
#include <stdbool.h>
#include "ufec23_protocol.h"
#include "ufec23_endec.h"

typedef struct 
{
    UFEC23ENDEC_S2CGetRunningSettingResp s2CGetRunningSetting;
    bool s2CGetRunningSettingIsSet;

    UFEC23ENDEC_S2CReqVersionResp sS2CReqVersionResp;
    bool sS2CReqVersionRespIsSet;

    // Config JSON
    uint8_t* pS2CConfigJSON;
    uint32_t u32S2CConfigJSONLen;
} STOVEMB_SMemBlock;

void STOVEMB_Init();

bool STOVEMB_Take(TickType_t xTicksToWait);

void STOVEMB_Give();

STOVEMB_SMemBlock* STOVEMB_GetMemBlock();

const STOVEMB_SMemBlock* STOVEMB_GetMemBlockRO();

char* STOVEMB_CopyServerParameterJSONTo();

#endif