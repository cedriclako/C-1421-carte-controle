#include "UIManager.h"
#include "Global.h"
#include "UI/MainUI.h"
#include "UI/PoweringOnUI.h"
#include "UI/SettingsUI.h"

#define TAG "UIManager"

static void SwitchUI(const COMMONUI_SContext* sContext, ESCREEN eScreen);

static MAINUI_SHandle m_sMainUIHandle; 
static MAINUI_SArgument m_sMainUIArgumentRO         = { .bIsUserModeActive = false };
static MAINUI_SArgument m_sMainUIArgumentUserMode   = { .bIsUserModeActive = true };

static COMMONUI_SUIManagerContext m_sUIManagerCtx = { .ptrSwitchUI = SwitchUI };

static COMMONUI_SContext m_sUIs[ESCREEN_Count] =
{
    // Main
    [ESCREEN_HomeReadOnly] = { .szName = "MainReadOnly", .pUIManagerCtx = &m_sUIManagerCtx, .pHandle = &m_sMainUIHandle, .psConfig = &MAINUI_g_sConfig, .pvdArgument = &m_sMainUIArgumentRO },
    [ESCREEN_HomeUsermode] = { .szName = "MainUsermode", .pUIManagerCtx = &m_sUIManagerCtx, .pHandle = &m_sMainUIHandle, .psConfig = &MAINUI_g_sConfig, .pvdArgument = &m_sMainUIArgumentUserMode },
    // Powering on
    [ESCREEN_PoweringOn]   = { .szName = "PoweringOn", .pUIManagerCtx = &m_sUIManagerCtx, .pHandle = NULL, .psConfig = &POWERINGONUI_g_sConfig },
    // Settings
    [ESCREEN_Settings]     = { .szName = "Settings", .pUIManagerCtx = &m_sUIManagerCtx, .pHandle = NULL, .psConfig = &SETTINGSUI_g_sConfig }
};

static ESCREEN m_eScreen = ESCREEN_Invalid;
static bool m_bIsFirstLoad = true;

void UIMANAGER_Init()
{
    ESP_LOGI(TAG, "CreateCanvas");
    G_g_CanvasResult.createCanvas(SCREEN_WIDTH, SCREEN_HEIGHT);
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