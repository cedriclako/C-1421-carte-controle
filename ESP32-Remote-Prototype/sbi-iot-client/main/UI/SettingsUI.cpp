#include "SettingsUI.h"

#define TAG "SettingsUI"

static void Init(COMMONUI_SContext* pContext);
static void Enter(COMMONUI_SContext* pContext);
static void Exit(COMMONUI_SContext* pContext);

const COMMONUI_SConfig SETTINGSUI_g_sConfig = 
{ 
    .ptrInit = Init, 
    .ptrEnter = Enter, 
    .ptrExit = Exit, 
};

static void Init(COMMONUI_SContext* pContext)
{

}

static void Enter(COMMONUI_SContext* pContext)
{
    ESP_LOGI(TAG, "Settings mode ...");
    G_g_CanvasResult.setTextSize(2);
    G_g_CanvasResult.setFreeFont(FF19);
    G_g_CanvasResult.drawCentreString("Settings mode ...", SCREEN_WIDTH/2, 850, GFXFF);
    G_g_CanvasResult.pushCanvas(0, 0, UPDATE_MODE_A2);
    G_g_CanvasResult.fillCanvas(0);
}

static void Exit(COMMONUI_SContext* pContext)
{

}
