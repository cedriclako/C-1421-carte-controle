#ifndef _STOVEMB_H_
#define _STOVEMB_H_

#include <stdint.h>
#include "ufec23_protocol.h"
#include "ufec23_endec.h"

typedef struct 
{
    UFEC23ENDEC_S2CGetRunningSettingResp s2CGetRunningSetting;
    bool s2CGetRunningSettingIsSet;
} STOVEMB_SMemBlock;

void STOVEMB_Init();

STOVEMB_SMemBlock* STOVEMB_GetMemBlock();

const STOVEMB_SMemBlock* STOVEMB_GetMemBlockRO();

#endif