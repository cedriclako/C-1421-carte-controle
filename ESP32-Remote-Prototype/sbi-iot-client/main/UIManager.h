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
    //UIMANAGER_ESCREEN_MainWrite,
    // UIMANAGER_ESCREEN_Setting,
    UIMANAGER_ESCREEN_PoweringOn,

    UIMANAGER_ESCREEN_Count
} UIMANAGER_ESCREEN;

void UIMANAGER_Init();

void UIMANAGER_SwitchTo(UIMANAGER_ESCREEN eScreen);

COMMONUI_SContext* UIMANAGER_GetUI();

#ifdef __cplusplus
}
#endif

#endif