#include "MainUI.h"
#include "Global.h"

#define TAG "MainUI"

static void Init(COMMONUI_SContext* pContext);

static void Enter(COMMONUI_SContext* pContext);
static void Exit(COMMONUI_SContext* pContext);

static void Process(COMMONUI_SContext* pContext);

static void OnTouch(COMMONUI_SContext* pContext, int32_t s32X, int32_t s32Y);

static void DrawAllBars(int32_t s32X, int32_t s32Y, uint32_t u32Bar);

static void RedrawUI(MAINUI_SHandle* pHandle);

const COMMONUI_SConfig MAINUI_g_sConfig = 
{ 
    .ptrInit = Init, 
    .ptrEnter = Enter, 
    .ptrExit = Exit, 
    .ptrProcess = Process, 
    .ptrOnTouch = OnTouch 
};

static void Init(COMMONUI_SContext* pContext)
{
    MAINUI_SHandle* pHandle = (MAINUI_SHandle*)pContext->pHandle;

    // Set point (UP)
    COMMONUI_SRect* pRectSetPointUp = &pHandle->sUIButtons[MAINUI_EBUTTONS_SetPointUp];
    pRectSetPointUp->u16X1 = 0;
    pRectSetPointUp->u16Y1 = 0;
    pRectSetPointUp->u16X2 = 0;
    pRectSetPointUp->u16Y2 = 0;
}

static void Enter(COMMONUI_SContext* pContext)
{   
    MAINUI_SHandle* pHandle = (MAINUI_SHandle*)pContext->pHandle;
    
    pHandle->bIsNeedClear = true;
    pHandle->u8CurrentFanSpeed = 1;
    pHandle->isUserModeActive = true;

    RedrawUI(pHandle);
}

static void Exit(COMMONUI_SContext* pContext)
{

}

static void OnTouch(COMMONUI_SContext* pContext, int32_t s32X, int32_t s32Y)
{

}

static void Process(COMMONUI_SContext* pContext)
{

}

static void RedrawUI(MAINUI_SHandle* pHandle)
{
    if (pHandle->bIsNeedClear) 
    {
        ESP_LOGI(TAG, "Clear EPD");
        M5.EPD.Clear(true);
    }
    
    // rtc_time_t RTCtime;
    // rtc_date_t RTCDate;
    // M5.RTC.getTime(&RTCtime);
    // M5.RTC.getDate(&RTCDate);

    float tem = M5.SHT30.GetTemperature();
    float hum = M5.SHT30.GetRelHumidity();
    char temStr[10];
    char humStr[10];
    dtostrf(tem, 2, 1, temStr);
    dtostrf(hum, 2, 1, humStr);

    // Images 
    const EF_SFile* pSFileArrowUp = &EF_g_sFiles[EF_EFILE_ICON_ARROW_UP_120X60_JPG];
    const EF_SImage* psMetaArrowUp = (const EF_SImage*)pSFileArrowUp->pMetaData;
    const EF_SFile* pSFileArrowDown = &EF_g_sFiles[EF_EFILE_ICON_ARROW_DOWN_120X60_JPG];
    const EF_SImage* psMetaArrowDown = (const EF_SImage*)pSFileArrowDown->pMetaData;
    const EF_SFile* pSFileSetting = &EF_g_sFiles[EF_EFILE_ICON_SETTING_160X160_JPG];
    const EF_SImage* psMetaSetting = (const EF_SImage*)pSFileSetting->pMetaData;
    const EF_SFile* pSFileSBILogo = &EF_g_sFiles[EF_EFILE_ICON_SBI_LOGO_152X112_JPG];
    const EF_SImage* psMetaSBILogo = (const EF_SImage*)pSFileSBILogo->pMetaData;

    // -----------------------------------
    // Current room temperature
    {
        const int32_t s32CurrentTempY = 0;

        G_g_CanvasResult.setFreeFont(FF18);
        G_g_CanvasResult.setTextSize(2);
        G_g_CanvasResult.drawCentreString("Room", 40+psMetaArrowUp->s32Width/2, s32CurrentTempY+96, GFXFF);

        G_g_CanvasResult.setFreeFont(FSSB18);
        G_g_CanvasResult.setTextSize(3);
        G_g_CanvasResult.drawString(String(temStr), 232, s32CurrentTempY+72);
        G_g_CanvasResult.setTextSize(1);
        G_g_CanvasResult.drawString("C", 440, s32CurrentTempY+72);
        
        G_g_CanvasResult.drawRect(40, s32CurrentTempY + 216, SCREEN_WIDTH-(40*2), 2, TFT_WHITE);
    }

    // -----------------------------------
    // Current set point
    {
        const int32_t s32CurrentTempY = 250;

        G_g_CanvasResult.setFreeFont(FF19);
        G_g_CanvasResult.setTextSize(1);
        G_g_CanvasResult.drawCentreString("Setpoint", 40+psMetaArrowUp->s32Width/2, s32CurrentTempY+92, GFXFF);
        if (pHandle->isUserModeActive)
        {
            G_g_CanvasResult.drawJpg(pSFileArrowUp->pu8StartAddr, pSFileArrowUp->u32Length, 40, s32CurrentTempY+16);
            G_g_CanvasResult.drawJpg(pSFileArrowDown->pu8StartAddr, pSFileArrowDown->u32Length, 40, s32CurrentTempY+136);
        }
        G_g_CanvasResult.setFreeFont(FSSB18);
        G_g_CanvasResult.setTextSize(3);
        G_g_CanvasResult.drawString(String(temStr), 232, s32CurrentTempY+72);
        G_g_CanvasResult.setTextSize(1);
        G_g_CanvasResult.drawString("C", 440, s32CurrentTempY+72);
        
        G_g_CanvasResult.drawRect(40, s32CurrentTempY + 216, SCREEN_WIDTH-(40*2), 2, TFT_WHITE);
    }

    // -----------------------------------
    // Current temperature
    {
        const int32_t s32CurrentTempY = 500;

        G_g_CanvasResult.setFreeFont(FF19);
        G_g_CanvasResult.setTextSize(1);
        G_g_CanvasResult.drawCentreString("Fan speed", 40+psMetaArrowUp->s32Width/2, s32CurrentTempY+92, GFXFF);
        if (pHandle->isUserModeActive)
        {
            G_g_CanvasResult.drawJpg(pSFileArrowUp->pu8StartAddr, pSFileArrowUp->u32Length, 40, s32CurrentTempY+16);
            G_g_CanvasResult.drawJpg(pSFileArrowDown->pu8StartAddr, pSFileArrowDown->u32Length, 40, s32CurrentTempY+136);
        }
        DrawAllBars(232, s32CurrentTempY, pHandle->u8CurrentFanSpeed);

        G_g_CanvasResult.drawRect(40, s32CurrentTempY + 216, SCREEN_WIDTH-(40*2), 2, TFT_WHITE);
    }

    if (pHandle->isUserModeActive)
    {
        G_g_CanvasResult.drawJpg(pSFileSetting->pu8StartAddr, pSFileSetting->u32Length, 344, 760);
    }

    G_g_CanvasResult.drawJpg(pSFileSBILogo->pu8StartAddr, pSFileSBILogo->u32Length, 16, 832);

    // Update screen
    if (pHandle->bIsNeedClear)
        G_g_CanvasResult.pushCanvas(0, 0, UPDATE_MODE_DU);
    else
        G_g_CanvasResult.pushCanvas(0, 0, UPDATE_MODE_DU4);

    pHandle->bIsNeedClear = false;
}

static void DrawAllBars(int32_t s32X, int32_t s32Y, uint32_t u32Bar)
{
    #define BAR_HEIGHT 192
    #define BAR_WIDTH 48
    #define BAR_MARGIN 16

    #define BAR_TOP(x) ((BAR_HEIGHT/32)*(32-x))
    #define DrawAllBars(_index, _level, _isActive) _isActive ? G_g_CanvasResult.fillRect(s32X+(BAR_WIDTH+BAR_MARGIN)*_index, s32Y + BAR_TOP(_level), BAR_WIDTH, BAR_HEIGHT - BAR_TOP(_level), TFT_WHITE) : G_g_CanvasResult.drawRect(s32X+(BAR_WIDTH+BAR_MARGIN)*_index, s32Y + BAR_TOP(_level), BAR_WIDTH, BAR_HEIGHT - BAR_TOP(_level), TFT_WHITE)
 
    /* 1/4 */DrawAllBars(0, 12, (u32Bar >= 1));
    /* 2/4 */DrawAllBars(1, 20, (u32Bar >= 2));
    /* 3/4 */DrawAllBars(2, 28, (u32Bar >= 3));
    /* 4/4 */DrawAllBars(3, 32, (u32Bar >= 4));
}
