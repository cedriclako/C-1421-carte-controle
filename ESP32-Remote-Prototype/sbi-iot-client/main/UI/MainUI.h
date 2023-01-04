#ifndef _MAINUI_H_
#define _MAINUI_H_

#include "CommonUI.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

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
    bool bIsUserModeActive;
} MAINUI_SArgument;

typedef struct
{
    COMMONUI_SRect sUIButtons[MAINUI_EBUTTONS_Count];

    // State
    bool bIsNeedClear;
    uint8_t u8CurrentFanSpeed;
    const MAINUI_SArgument* pArgument;
} MAINUI_SHandle;

extern const COMMONUI_SConfig MAINUI_g_sConfig;

#ifdef __cplusplus
}
#endif

#endif