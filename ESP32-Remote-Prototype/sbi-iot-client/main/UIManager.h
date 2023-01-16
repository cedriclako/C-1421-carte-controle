#ifndef _UIMANAGER_H_
#define _UIMANAGER_H_

#include "UI/CommonUI.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    UIMANAGER_ESCREEN_Invalid = -1,  // Default value, nothing loaded yet.

    UIMANAGER_ESCREEN_MainReadOnly = 0,
    UIMANAGER_ESCREEN_MainUsermode,
    
    UIMANAGER_ESCREEN_PoweringOn,

    UIMANAGER_ESCREEN_Settings,

    UIMANAGER_ESCREEN_Count
} UIMANAGER_ESCREEN;

void UIMANAGER_Init();

void UIMANAGER_Process();

void UIMANAGER_OnTouch(int32_t s32X, int32_t s32Y);

void UIMANAGER_SwitchTo(UIMANAGER_ESCREEN eScreen);

COMMONUI_SContext* UIMANAGER_GetUI();

#ifdef __cplusplus
}
#endif

#endif