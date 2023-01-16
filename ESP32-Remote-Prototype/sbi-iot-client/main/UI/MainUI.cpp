#include "MainUI.h"
#include "Global.h"
#include "../UIManager.h"

#define TAG "MainUI"

#define BAR_HEIGHT 192
#define BAR_WIDTH 48
#define BAR_MARGIN 16

#define ZONE_SETPOINT_START_Y 250
#define ZONE_SETPOINT_BUTTONUP_X (40)
#define ZONE_SETPOINT_BUTTONUP_Y (ZONE_SETPOINT_START_Y+16)
#define ZONE_SETPOINT_BUTTONDOWN_X (40)
#define ZONE_SETPOINT_BUTTONDOWN_Y (ZONE_SETPOINT_START_Y+136)

#define ZONE_FANSPEED_START_Y 500
#define ZONE_FANSPEED_BUTTONUP_X (40)
#define ZONE_FANSPEED_BUTTONUP_Y (ZONE_FANSPEED_START_Y+16)
#define ZONE_FANSPEED_BUTTONDOWN_X (40)
#define ZONE_FANSPEED_BUTTONDOWN_Y (ZONE_FANSPEED_START_Y+136)

#define ZONE_BTSETTING_START_X 344
#define ZONE_BTSETTING_START_Y 760

static void Init(COMMONUI_SContext* pContext);

static void Enter(COMMONUI_SContext* pContext);
static void Exit(COMMONUI_SContext* pContext);

static void Process(COMMONUI_SContext* pContext);

static void OnTouch(COMMONUI_SContext* pContext, int32_t s32X, int32_t s32Y);

static void DrawAllBars(int32_t s32X, int32_t s32Y, uint32_t u32Bar);
static void DrawBar(int32_t s32X, int32_t s32Y, int32_t s32Level, bool bIsActive);

static void RedrawUI(COMMONUI_SContext* pContext);

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
    
    pHandle->u8CurrentFanSpeed = 1;
    pHandle->fSetPoint = 20.0;

    RedrawUI(pContext);
}

static void Exit(COMMONUI_SContext* pContext)
{

}

static void OnTouch(COMMONUI_SContext* pContext, int32_t s32TouchX, int32_t s32TouchY)
{
    const MAINUI_SArgument* pArgument = (const MAINUI_SArgument*)pContext->pvdArgument;
    if (!pArgument->bIsUserModeActive)
    {
        // Read-Only mode.
        return;
    }

    MAINUI_SHandle* pHandle = (MAINUI_SHandle*)pContext->pHandle;

    bool bNeedRedraw = false;

    // ==================================
    // Set point
    // Configs fan speed
    if ( COMMONUI_IsInCoordinate(ZONE_SETPOINT_BUTTONDOWN_X, ZONE_SETPOINT_BUTTONDOWN_Y, EF_g_sIMAGES_ICON_ARROW_UP_EN_120X60_JPG.s32Width, EF_g_sIMAGES_ICON_ARROW_UP_EN_120X60_JPG.s32Height, s32TouchX, s32TouchY) )
    {
        bNeedRedraw = true;
        if (pHandle->fSetPoint - 0.5f >= 5.0f)
            pHandle->fSetPoint -= 0.5f;
    }
    else if ( COMMONUI_IsInCoordinate(ZONE_SETPOINT_BUTTONUP_X, ZONE_SETPOINT_BUTTONUP_Y, EF_g_sIMAGES_ICON_ARROW_UP_EN_120X60_JPG.s32Width, EF_g_sIMAGES_ICON_ARROW_UP_EN_120X60_JPG.s32Height, s32TouchX, s32TouchY) )
    {
        bNeedRedraw = true;
        if (pHandle->fSetPoint + 0.5f <= 40.0f)
            pHandle->fSetPoint += 0.5f;
    }

    // ==================================
    // Configs fan speed
    if ( COMMONUI_IsInCoordinate(ZONE_FANSPEED_BUTTONDOWN_X, ZONE_FANSPEED_BUTTONDOWN_Y, EF_g_sIMAGES_ICON_ARROW_UP_EN_120X60_JPG.s32Width, EF_g_sIMAGES_ICON_ARROW_UP_EN_120X60_JPG.s32Height, s32TouchX, s32TouchY) )
    {
        bNeedRedraw = true;
        if (pHandle->u8CurrentFanSpeed - 1 >= 1)
            pHandle->u8CurrentFanSpeed--;
    }
    else if ( COMMONUI_IsInCoordinate(ZONE_FANSPEED_BUTTONUP_X, ZONE_FANSPEED_BUTTONUP_Y, EF_g_sIMAGES_ICON_ARROW_UP_EN_120X60_JPG.s32Width, EF_g_sIMAGES_ICON_ARROW_UP_EN_120X60_JPG.s32Height, s32TouchX, s32TouchY) )
    {
        bNeedRedraw = true;
        if (pHandle->u8CurrentFanSpeed + 1 <= 4)
            pHandle->u8CurrentFanSpeed++;
    }

    if ( COMMONUI_IsInCoordinate(ZONE_BTSETTING_START_X, ZONE_BTSETTING_START_Y, EF_g_sIMAGES_ICON_SETTING_160X160_JPG.s32Width, EF_g_sIMAGES_ICON_SETTING_160X160_JPG.s32Height, s32TouchX, s32TouchY) )
    {
        pContext->pUIManagerCtx->ptrSwitchUI(pContext, ESCREEN_Settings);
    }

    ESP_LOGI(TAG, "Touch x: %d, y: %d", s32TouchX, s32TouchY);

    if (bNeedRedraw)
        RedrawUI(pContext);
}

static void Process(COMMONUI_SContext* pContext)
{

}

static void RedrawUI(COMMONUI_SContext* pContext)
{
    const MAINUI_SArgument* pArgument = (const MAINUI_SArgument*)pContext->pvdArgument;
    MAINUI_SHandle* pHandle = (MAINUI_SHandle*)pContext->pHandle;

    // rtc_time_t RTCtime;
    // rtc_date_t RTCDate;
    // M5.RTC.getTime(&RTCtime);
    // M5.RTC.getDate(&RTCDate);
    float tem = M5.SHT30.GetTemperature();
    //float hum = M5.SHT30.GetRelHumidity();
    char tmp[40+1];
    //char humStr[10];
    //dtostrf(tem, 2, 1, temStr);
    //dtostrf(hum, 2, 1, humStr);

    // Images
    const EF_SFile* pSFileSetting = &EF_g_sFiles[EF_EFILE_ICON_SETTING_160X160_JPG];
    const EF_SFile* pSFileSBILogo = &EF_g_sFiles[EF_EFILE_ICON_SBI_LOGO_152X112_JPG];

    // -----------------------------------
    // Current room temperature
    {
        const int32_t s32CurrentTempY = 0;

        G_g_CanvasResult.setFreeFont(FF18);
        G_g_CanvasResult.setTextSize(2);
        G_g_CanvasResult.drawCentreString("Room", 40+EF_g_sIMAGES_ICON_ARROW_DOWN_EN_120X60_JPG.s32Width/2, s32CurrentTempY+96, GFXFF);

        G_g_CanvasResult.setFreeFont(FSSB18);
        G_g_CanvasResult.setTextSize(3);
        sprintf(tmp, "%.1f", pHandle->fSetPoint);
        G_g_CanvasResult.drawString(String(tmp), 232, s32CurrentTempY+72);
        G_g_CanvasResult.setTextSize(1);
        G_g_CanvasResult.drawString("C", 440, s32CurrentTempY+72);
        
        G_g_CanvasResult.drawRect(40, s32CurrentTempY + 216, SCREEN_WIDTH-(40*2), 2, TFT_WHITE);
    }

    // -----------------------------------
    // Current set point
    {
        G_g_CanvasResult.setFreeFont(FF19);
        G_g_CanvasResult.setTextSize(1);
        G_g_CanvasResult.drawCentreString("Setpoint", 40+EF_g_sIMAGES_ICON_ARROW_DOWN_EN_120X60_JPG.s32Width/2, ZONE_SETPOINT_START_Y+92, GFXFF);
        if (pArgument->bIsUserModeActive)
        {
            const EF_SFile* pSFileArrowUp = COMMONUI_GetBtnArrowUp(true);
            G_g_CanvasResult.drawJpg(pSFileArrowUp->pu8StartAddr, pSFileArrowUp->u32Length, ZONE_SETPOINT_BUTTONUP_X, ZONE_SETPOINT_BUTTONUP_Y);
            const EF_SFile* pSFileArrowDown = COMMONUI_GetBtnArrowDown(true);
            G_g_CanvasResult.drawJpg(pSFileArrowDown->pu8StartAddr, pSFileArrowDown->u32Length, ZONE_SETPOINT_BUTTONDOWN_X, ZONE_SETPOINT_BUTTONDOWN_Y);
        }
        G_g_CanvasResult.setFreeFont(FSSB18);
        G_g_CanvasResult.setTextSize(3);
        sprintf(tmp, "%.1f", pHandle->fSetPoint);
        G_g_CanvasResult.drawString(String(tmp), 232, ZONE_SETPOINT_START_Y+72);
        G_g_CanvasResult.setTextSize(1);
        G_g_CanvasResult.drawString("C", 440, ZONE_SETPOINT_START_Y+72);
        
        G_g_CanvasResult.drawRect(40, ZONE_SETPOINT_START_Y + 216, SCREEN_WIDTH-(40*2), 2, TFT_WHITE);
    }

    // -----------------------------------
    // Current temperature
    {
        G_g_CanvasResult.setFreeFont(FF19);
        G_g_CanvasResult.setTextSize(1);
        G_g_CanvasResult.drawCentreString("Fan speed", 40+EF_g_sIMAGES_ICON_ARROW_DOWN_EN_120X60_JPG.s32Width/2, ZONE_FANSPEED_START_Y+92, GFXFF);
        if (pArgument->bIsUserModeActive)
        {
            const EF_SFile* pSFileArrowUp = COMMONUI_GetBtnArrowUp(pHandle->u8CurrentFanSpeed != 4);
            G_g_CanvasResult.drawJpg(pSFileArrowUp->pu8StartAddr, pSFileArrowUp->u32Length, ZONE_FANSPEED_BUTTONUP_X, ZONE_FANSPEED_BUTTONUP_Y);

            const EF_SFile* pSFileArrowDown = COMMONUI_GetBtnArrowDown(pHandle->u8CurrentFanSpeed != 1);
            G_g_CanvasResult.drawJpg(pSFileArrowDown->pu8StartAddr, pSFileArrowDown->u32Length, ZONE_FANSPEED_BUTTONDOWN_X, ZONE_FANSPEED_BUTTONDOWN_Y);
        }
        DrawAllBars(232, ZONE_FANSPEED_START_Y, pHandle->u8CurrentFanSpeed);

        G_g_CanvasResult.drawRect(40, ZONE_FANSPEED_START_Y + 216, SCREEN_WIDTH-(40*2), 2, TFT_WHITE);
    }

    // -----------------------------------
    // Settings
    if (pArgument->bIsUserModeActive)
    {
        G_g_CanvasResult.drawJpg(pSFileSetting->pu8StartAddr, pSFileSetting->u32Length, ZONE_BTSETTING_START_X, ZONE_BTSETTING_START_Y);
    }

    G_g_CanvasResult.drawJpg(pSFileSBILogo->pu8StartAddr, pSFileSBILogo->u32Length, 16, 832);

    // Update screen
    G_g_CanvasResult.pushCanvas(0, 0, UPDATE_MODE_DU4);
}

static void DrawAllBars(int32_t s32X, int32_t s32Y, uint32_t u32Bar)
{
    #define BAR_OFFSETX(_index) ((BAR_WIDTH+BAR_MARGIN)*_index)

    /* 1/4 */DrawBar(s32X + BAR_OFFSETX(0), s32Y, 12, (u32Bar >= 1));
    /* 2/4 */DrawBar(s32X + BAR_OFFSETX(1), s32Y, 20, (u32Bar >= 2));
    /* 3/4 */DrawBar(s32X + BAR_OFFSETX(2), s32Y, 28, (u32Bar >= 3));
    /* 4/4 */DrawBar(s32X + BAR_OFFSETX(3), s32Y, 32, (u32Bar >= 4));
}

static void DrawBar(int32_t s32X, int32_t s32Y, int32_t s32Level, bool bIsActive)
{  
    #define BAR_TOP(x) ((BAR_HEIGHT/32)*(32-x))
    
    G_g_CanvasResult.fillRect(s32X, s32Y + BAR_TOP(s32Level), BAR_WIDTH, BAR_HEIGHT - BAR_TOP(s32Level), (bIsActive ? TFT_WHITE : TFT_BLACK));
    G_g_CanvasResult.drawRect(s32X, s32Y + BAR_TOP(s32Level), BAR_WIDTH, BAR_HEIGHT - BAR_TOP(s32Level), TFT_WHITE);
}