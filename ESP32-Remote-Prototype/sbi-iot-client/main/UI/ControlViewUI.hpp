#ifndef _CONTROLVIEWUI_H_
#define _CONTROLVIEWUI_H_

#include "CommonUI.hpp"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    CONTROLVIEWUI_EBUTTONS_SetPointUp = 0,
    CONTROLVIEWUI_EBUTTONS_SetPointDn,
    
    CONTROLVIEWUI_EBUTTONS_FanSpeedChange,
    CONTROLVIEWUI_EBUTTONS_ActionDistribution,
    CONTROLVIEWUI_EBUTTONS_ActionFireBoost,
    
    CONTROLVIEWUI_EBUTTONS_Count
} CONTROLVIEWUI_EBUTTONS;

typedef struct
{
    bool bIsUserModeActive;
} CONTROLVIEWUI_SArgument;

typedef struct
{
    COMMONUI_SButton sUIButtons[CONTROLVIEWUI_EBUTTONS_Count];

    // State
    const CONTROLVIEWUI_SArgument* pArgument;
} CONTROLVIEWUI_SHandle;

extern const COMMONUI_SConfig CONTROLVIEWUI_g_sConfig;

#ifdef __cplusplus
}
#endif

#endif