#include "SettingsUI.h"

#define TAG "SettingsUI"

#define ZONE_BTBACK_START_X 36
#define ZONE_BTBACK_START_Y 760

static void Init(COMMONUI_SContext* pContext);
static void Enter(COMMONUI_SContext* pContext);
static void Exit(COMMONUI_SContext* pContext);

static void Process(COMMONUI_SContext* pContext);

static void OnTouch(COMMONUI_SContext* pContext, int32_t s32X, int32_t s32Y);

const COMMONUI_SConfig SETTINGSUI_g_sConfig = 
{ 
    .ptrInit = Init, 
    .ptrEnter = Enter, 
    .ptrExit = Exit, 
    
    .ptrProcess = Process, 
    .ptrOnTouch = OnTouch 
};

static void Init(COMMONUI_SContext* pContext)
{

}

static void Enter(COMMONUI_SContext* pContext)
{
    ESP_LOGI(TAG, "Settings mode ...");
    G_g_CanvasResult.setTextSize(2);
    G_g_CanvasResult.setFreeFont(FF19);
    G_g_CanvasResult.drawCentreString("Settings mode ...", SCREEN_WIDTH/2, 350, GFXFF);

    const EF_SFile* pSFileBtBack = &EF_g_sFiles[EF_EFILE_ICON_ARROW_BACK_EN_160X160_JPG];
    G_g_CanvasResult.drawJpg(pSFileBtBack->pu8StartAddr, pSFileBtBack->u32Length, ZONE_BTBACK_START_X, ZONE_BTBACK_START_Y);
    G_g_CanvasResult.pushCanvas(0, 0, UPDATE_MODE_DU4);
}

static void Exit(COMMONUI_SContext* pContext)
{

}

static void Process(COMMONUI_SContext* pContext)
{

}

static void OnTouch(COMMONUI_SContext* pContext, int32_t s32TouchX, int32_t s32TouchY)
{
    if ( COMMONUI_IsInCoordinate(ZONE_BTBACK_START_X, ZONE_BTBACK_START_Y, EF_g_sIMAGES_ICON_ARROW_BACK_EN_160X160_JPG.s32Width, EF_g_sIMAGES_ICON_ARROW_BACK_EN_160X160_JPG.s32Height, s32TouchX, s32TouchY) )
    {
        pContext->pUIManagerCtx->ptrSwitchUI(pContext, ESCREEN_HomeUsermode);
    }
}