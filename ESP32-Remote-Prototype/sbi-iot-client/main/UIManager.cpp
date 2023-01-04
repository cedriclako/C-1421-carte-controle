#include "UIManager.h"
#include "Global.h"
#include "UI/MainUI.h"
#include "UI/PoweringOnUI.h"

#define TAG "UIManager"

static MAINUI_SHandle m_sMainUIHandle; 

static COMMONUI_SContext m_sUIs[UIMANAGER_ESCREEN_Count] =
{
    [UIMANAGER_ESCREEN_MainReadOnly] = { .pHandle = &m_sMainUIHandle, .psConfig = &MAINUI_g_sConfig },
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