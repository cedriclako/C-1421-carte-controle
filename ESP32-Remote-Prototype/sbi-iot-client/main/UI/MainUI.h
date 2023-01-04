#ifndef _MAINUI_H_
#define _MAINUI_H_

#include "CommonUI.h"

typedef enum
{
    MAINUI_EBUTTONS_SetPointUp = 0,
    MAINUI_EBUTTONS_SetPointDn,
    
    MAINUI_EBUTTONS_FanSpeedUp,
    MAINUI_EBUTTONS_FanSpeedDn,
    
    MAINUI_EBUTTONS_Setting,

    MAINUI_EBUTTONS_Count
} EBUTTONS;

typedef struct
{
    COMMONUI_SRect sUIButtons[MAINUI_EBUTTONS_Count];

    const COMMONUI_SConfig* psConfig;
} MAINUI_SHandle;

void MAINUI_Init(MAINUI_SHandle* pContext);

void MAINUI_Process(MAINUI_SHandle* pContext);

void MAINUI_OnTouch(MAINUI_SHandle* pContext, int32_t s32X, int32_t s32Y);

#endif