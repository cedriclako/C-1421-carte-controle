#ifndef _OTACHECK_H_
#define _OTACHECK_H_

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define OTACHECK_SOTAITEM_COUNT (4)

typedef struct
{
    uint32_t u32Id;
    char szURL[180+1];
    char szVersion[12+1]; // 000.000.000
    char szChangeLogs[250+1];
} OTACHECK_SOTAItem;

typedef enum
{
    OTACHECK_ERESULT_Idle = 0,
    OTACHECK_ERESULT_Progress = 1,
    OTACHECK_ERESULT_Error = 2,
    OTACHECK_ERESULT_Success = 3
} OTACHECK_ERESULT;

typedef struct
{
    OTACHECK_ERESULT eResult;
    double dProgressOfOne;
} OTACHECK_SProgress;

void OTACHECK_Init();

bool OTACHECK_CheckOTAvailability(uint32_t u32TimeoutMS);

bool OTACHECK_InstallOTA(uint32_t u32Id, uint32_t u32TimeoutMS);

// Thread safe
void OTACHECK_OTAItemBegin();
// Thread safe
bool OTACHECK_OTAItemGet(OTACHECK_SOTAItem* pOut); 

OTACHECK_SProgress OTACHECK_GetProgress();

#endif