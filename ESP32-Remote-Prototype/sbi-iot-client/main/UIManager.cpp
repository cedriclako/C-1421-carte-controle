#include "UIManager.h"
#include "Global.h"
#include "UI/MainUI.h"
#include "UI/PoweringOnUI.h"

#define TAG "UIManager"

static MAINUI_SHandle m_sMainUIHandle; 
static MAINUI_SArgument m_sMainUIArgumentRO         = { .bIsUserModeActive = false };
static MAINUI_SArgument m_sMainUIArgumentUserMode   = { .bIsUserModeActive = true };

static COMMONUI_SContext m_sUIs[UIMANAGER_ESCREEN_Count] =
{
    // Main
    [UIMANAGER_ESCREEN_MainReadOnly] = { .pHandle = &m_sMainUIHandle, .psConfig = &MAINUI_g_sConfig, .pvdArgument = &m_sMainUIArgumentRO },
    [UIMANAGER_ESCREEN_MainUsermode] = { .pHandle = &m_sMainUIHandle, .psConfig = &MAINUI_g_sConfig, .pvdArgument = &m_sMainUIArgumentUserMode },
    // Powering on
    [UIMANAGER_ESCREEN_PoweringOn] = { .pHandle = NULL, .psConfig = &POWERINGONUI_g_sConfig }
};

static UIMANAGER_ESCREEN m_eScreen = UIMANAGER_ESCREEN_Invalid;

void UIMANAGER_Init()
{
    ESP_LOGI(TAG, "CreateCanvas");
    G_g_CanvasResult.createCanvas(SCREEN_WIDTH, SCREEN_HEIGHT);
}

void UIMANAGER_SwitchTo(UIMANAGER_ESCREEN eScreen)
{
    // Call exit on older process ...
    COMMONUI_SContext* pOldContext = UIMANAGER_GetUI();
    if (pOldContext != NULL && pOldContext->psConfig->ptrExit != NULL)
        pOldContext->psConfig->ptrExit(pOldContext);

    m_eScreen = eScreen;
    COMMONUI_SContext* pContext = UIMANAGER_GetUI();
    if (pContext != NULL && pContext->psConfig->ptrEnter != NULL)
        pContext->psConfig->ptrEnter(pContext);
}

COMMONUI_SContext* UIMANAGER_GetUI()
{
    if (m_eScreen == UIMANAGER_ESCREEN_Invalid)
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