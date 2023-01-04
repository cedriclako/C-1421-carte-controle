#include "MainUI.h"

void MAINUI_Init(MAINUI_SHandle* pContext)
{
    // Set point (UP)
    SRect* pRectSetPointUp = &pContext->sUIButtons[MAINUI_EBUTTONS_SetPointUp];
    pRectSetPointUp->u16X1 = 0;
    pRectSetPointUp->u16Y1 = 0;
    pRectSetPointUp->u16X2 = 0;
    pRectSetPointUp->u16Y2 = 0;
}

void MAINUI_Process(MAINUI_SHandle* pContext)
{

}

void MAINUI_OnTouch(MAINUI_SHandle* pContext, int32_t s32X, int32_t s32Y)
{

}