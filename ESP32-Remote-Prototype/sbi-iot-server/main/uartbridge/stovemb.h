#ifndef _STOVEMB_H_
#define _STOVEMB_H_

#include <stdint.h>
#include "ufec23_protocol.h"
#include "ufec23_endec.h"

typedef struct 
{
    UFEC23ENDEC_S2CGetRunningSettingResp s2CGetRunningSetting;
} STOVEMB_Sa;

void STOVEMB_Init();

#endif