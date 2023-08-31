#include "UIManager.hpp"
#include "Global.h"
#include "UI/ControlViewUI.hpp"
#include "UI/SettingsUI.hpp"
#include "UI/PoweringOnViewUI.hpp"
#include "UI/HomeViewUI.hpp"

#define TAG "UIManager"

static void SwitchUI(const COMMONUI_SContext* sContext, ESCREEN eScreen);

static CONTROLVIEWUI_SHandle m_sMainUIHandle;
static SETTINGSUI_SHandle m_sSettingsUIHandle;

static CONTROLVIEWUI_SArgument m_sMainUIArgumentUserMode   = { .bIsUserModeActive = true };

static COMMONUI_SUIManagerContext m_sUIManagerCtx = { .ptrSwitchUI = SwitchUI };

static COMMONUI_SContext m_sUIs[ESCREEN_Count] =
{
    // Main
    [ESCREEN_ControlViewUI]          = { .szName = "ControlViewUI", .pUIManagerCtx = &m_sUIManagerCtx, .pHandle = &m_sMainUIHandle, .psConfig = &CONTROLVIEWUI_g_sConfig, .pvdArgument = &m_sMainUIArgumentUserMode },
    // Home off
    [ESCREEN_HomeViewUI]             = { .szName = "HomeViewUI", .pUIManagerCtx = &m_sUIManagerCtx, .pHandle = NULL, .psConfig = &HOMEVIEWUI_g_sConfig },
    // Powering on
    [ESCREEN_PoweringOn]            = { .szName = "PoweringOn", .pUIManagerCtx = &m_sUIManagerCtx, .pHandle = NULL, .psConfig = &POWERINGONVIEWUI_g_sConfig },
    // Settings
    [ESCREEN_Settings]              = { .szName = "Settings", .pUIManagerCtx = &m_sUIManagerCtx, .pHandle = &m_sSettingsUIHandle, .psConfig = &SETTINGSUI_g_sConfig }
};

static ESCREEN m_eScreen = ESCREEN_Invalid;
static bool m_bIsFirstLoad = true;

void UIMANAGER_Init()
{
    ESP_LOGI(TAG, "CreateCanvas");
    G_g_CanvasResult.createCanvas(SCREEN_WIDTH, SCREEN_HEIGHT);

    for(int i = 0; i < ESCREEN_Count; i++)
    {
        COMMONUI_SContext* pContext = &m_sUIs[i];
        if (pContext->psConfig != NULL && pContext->psConfig->ptrInit != NULL)
        {
            pContext->psConfig->ptrInit(pContext);
        }
    }
}

void UIMANAGER_SwitchTo(ESCREEN eScreen)
{
    // Already loaded in the right screen ...
    if (m_eScreen == eScreen)
        return;

    G_g_CanvasResult.fillCanvas(TFT_BLACK);
    if (m_bIsFirstLoad)
    {
        M5.EPD.Clear(true);
        m_bIsFirstLoad = false;
    }

    // Call exit on older process ...
    COMMONUI_SContext* pOldContext = UIMANAGER_GetUI();
    if (pOldContext != NULL && pOldContext->psConfig->ptrExit != NULL)
        pOldContext->psConfig->ptrExit(pOldContext);

    m_eScreen = eScreen;
    COMMONUI_SContext* pContext = UIMANAGER_GetUI();

    ESP_LOGI(TAG, "switch from %s to %s", (pOldContext != NULL ? pOldContext->szName : "None"), pContext->szName);

    if (pContext != NULL && pContext->psConfig->ptrEnter != NULL)
        pContext->psConfig->ptrEnter(pContext);
}

COMMONUI_SContext* UIMANAGER_GetUI()
{
    if (m_eScreen == ESCREEN_Invalid)
        return NULL;
    return &m_sUIs[(int)m_eScreen];
}

void UIMANAGER_Process()
{
    COMMONUI_SContext* pContext = UIMANAGER_GetUI();
    if (pContext != NULL && pContext->psConfig->ptrProcess != NULL)
        pContext->psConfig->ptrProcess(pContext);
}

void UIMANAGER_OnTouch(int32_t s32X, int32_t s32Y)
{
    COMMONUI_SContext* pContext = UIMANAGER_GetUI();
    if (pContext != NULL && pContext->psConfig->ptrOnTouch != NULL)
        pContext->psConfig->ptrOnTouch(pContext, s32X, s32Y);
}

void UIMANAGER_OnDataReceived()
{
    COMMONUI_SContext* pContext = UIMANAGER_GetUI();
    if (pContext != NULL && pContext->psConfig->ptrOnDataReceived != NULL)
        pContext->psConfig->ptrOnDataReceived(pContext);
}

static void SwitchUI(const COMMONUI_SContext* sContext, ESCREEN eScreen)
{
    UIMANAGER_SwitchTo(eScreen);
}