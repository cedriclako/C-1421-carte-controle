#ifndef _UIMANAGER_H_
#define _UIMANAGER_H_

#include "UI/CommonUI.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void UIMANAGER_Init();

void UIMANAGER_Process();

void UIMANAGER_OnTouch(int32_t s32X, int32_t s32Y);

void UIMANAGER_SwitchTo(ESCREEN eScreen);

COMMONUI_SContext* UIMANAGER_GetUI();

void UIMANAGER_OnDataReceived();

#ifdef __cplusplus
}
#endif

#endif