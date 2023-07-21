#ifndef _COMMONUI_H_
#define _COMMONUI_H_

#include "EScreen.hpp"
#include <M5EPD.h>
#include <stdint.h>
#include "../assets/EmbeddedFiles.h"
#include "esp_log.h"
#include "../Global.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SCREEN_WIDTH 540
#define SCREEN_HEIGHT 960

typedef struct _COMMONUI_SConfig COMMONUI_SConfig;
typedef struct _COMMONUI_SUIManagerContext COMMONUI_SUIManagerContext;

typedef struct 
{
    const char* szName;
    COMMONUI_SUIManagerContext* pUIManagerCtx;
    void* pHandle;
    const COMMONUI_SConfig* psConfig;
    void* pvdArgument;
} COMMONUI_SContext;

// Screen events
typedef void (*COMMONUI_InitFn)(COMMONUI_SContext* pContext);

typedef void (*COMMONUI_EnterFn)(COMMONUI_SContext* pContext);
typedef void (*COMMONUI_ExitFn)(COMMONUI_SContext* pContext);

typedef void (*COMMONUI_ProcessFn)(COMMONUI_SContext* pContext);
typedef void (*COMMONUI_OnTouchFn)(COMMONUI_SContext* pContext, int32_t s32TouchX, int32_t s32TouchY);

typedef void (*COMMONUI_OnDataReceivedFn)(COMMONUI_SContext* pContext);

// Core events
typedef void (*COMMONUI_SwitchUIFn)(const COMMONUI_SContext* pContext, ESCREEN eScreen);

typedef struct
{
    int32_t s32X;
    int32_t s32Y;
    int32_t s32Width;
    int32_t s32Height;
} COMMONUI_SRect;

typedef struct
{
    const EF_SFile* pSFile;
    COMMONUI_SRect sRect;
} COMMONUI_SButton;

struct _COMMONUI_SConfig
{
    // Function pointer
    COMMONUI_InitFn ptrInit;

    COMMONUI_EnterFn ptrEnter;
    COMMONUI_ExitFn ptrExit;

    COMMONUI_ProcessFn ptrProcess;
    COMMONUI_OnTouchFn ptrOnTouch;

    COMMONUI_OnDataReceivedFn ptrOnDataReceived;
};

struct _COMMONUI_SUIManagerContext
{
    COMMONUI_SwitchUIFn ptrSwitchUI;
};

const EF_SFile* COMMONUI_GetBtnArrowUp(bool bIsEnabled);
const EF_SFile* COMMONUI_GetBtnArrowDown(bool bIsEnabled);

bool COMMONUI_IsInCoordinate(const COMMONUI_SButton* psButton, int32_t s32TouchX, int32_t s32TouchY);

void COMMONUI_Button_Init(COMMONUI_SButton* sButton, const EF_SFile* pSFile, int32_t s32X, int32_t s32Y);

void COMMONUI_Button_Draw(const COMMONUI_SButton* psButton);

#ifdef __cplusplus
}
#endif

#endif