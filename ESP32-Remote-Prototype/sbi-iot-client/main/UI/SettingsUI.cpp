#include "SettingsUI.hpp"

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
    SETTINGSUI_SHandle* pHandle = (SETTINGSUI_SHandle*)pContext->pHandle;

    COMMONUI_Button_Init(&pHandle->sBtClose, &EF_g_sFiles[EF_EFILE_ICON_ARROW_BACK_EN_160X160_JPG], ZONE_BTBACK_START_X, ZONE_BTBACK_START_Y);
}

static void Enter(COMMONUI_SContext* pContext)
{
    SETTINGSUI_SHandle* pHandle = (SETTINGSUI_SHandle*)pContext->pHandle;

    ESP_LOGI(TAG, "Settings mode ...");
    G_g_CanvasResult.setTextSize(2);
    G_g_CanvasResult.setFreeFont(FF19);
    G_g_CanvasResult.drawCentreString("Settings mode ...", SCREEN_WIDTH/2, 350, GFXFF);

    COMMONUI_Button_Draw(&pHandle->sBtClose);
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
    SETTINGSUI_SHandle* pHandle = (SETTINGSUI_SHandle*)pContext->pHandle;
    if ( COMMONUI_IsInCoordinate(&pHandle->sBtClose, s32TouchX, s32TouchY) )
    {
        pContext->pUIManagerCtx->ptrSwitchUI(pContext, ESCREEN_ControlViewUI);
    }
}