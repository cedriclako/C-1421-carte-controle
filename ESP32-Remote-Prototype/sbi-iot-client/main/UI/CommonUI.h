#ifndef _COMMONUI_H_
#define _COMMONUI_H_

#include <stdint.h>

typedef void (*COMMONUI_InitFn)(void* pContext);
typedef void (*COMMONUI_ProcessFn)(void* pContext);
typedef void (*COMMONUI_OnTouchFn)(void* pContext, int32_t s32X, int32_t s32Y);

typedef struct
{
    EBUTTONS eBtnId;

    uint16_t u16X1;
    uint16_t u16Y1;
    uint16_t u16X2;
    uint16_t u16Y2;
} COMMONUI_SRect;

typedef struct
{
    // Function pointer
    COMMONUI_InitFn ptrInit;
    COMMONUI_ProcessFn ptrProcess;
    COMMONUI_OnTouchFn ptrOnTouch;
} COMMONUI_SConfig;

#endif