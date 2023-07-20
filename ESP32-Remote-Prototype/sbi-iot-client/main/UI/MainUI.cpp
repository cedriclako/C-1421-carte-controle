#include "MainUI.hpp"
#include "Global.h"
#include "../MemBlock.h"
#include "../UIManager.hpp"
#include "espnowcomm.h"

#define TAG "MainUI"

#define BAR_HEIGHT 192
#define BAR_WIDTH 48
#define BAR_MARGIN 16

#define ZONE_SETPOINT_START_Y (15)
#define ZONE_SETPOINT_BUTTONUP_X (360)
#define ZONE_SETPOINT_BUTTONUP_Y (ZONE_SETPOINT_START_Y+15)
#define ZONE_SETPOINT_BUTTONDOWN_X (360)
#define ZONE_SETPOINT_BUTTONDOWN_Y (ZONE_SETPOINT_START_Y+80)

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

static void OnDataReceived(COMMONUI_SContext* pContext);

static void DrawAllBars(int32_t s32X, int32_t s32Y, uint32_t u32Bar);
static void DrawBar(int32_t s32X, int32_t s32Y, int32_t s32Level, bool bIsActive);

static void RedrawUI(COMMONUI_SContext* pContext);

const COMMONUI_SConfig MAINUI_g_sConfig = 
{ 
    .ptrInit = Init, 
    .ptrEnter = Enter, 
    .ptrExit = Exit, 
    .ptrProcess = Process, 
    .ptrOnTouch = OnTouch,
    .ptrOnDataReceived = OnDataReceived
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
        if (g_sMemblock.s2cGetStatusResp.stove_state.remote_temperature_setp.temp - 0.5f >= 5.0f)
            g_sMemblock.s2cGetStatusResp.stove_state.remote_temperature_setp.temp -= 0.5f;
        g_sMemblock.isTemperatureSetPointChanged = true;
        ESPNOWCOMM_SendChangeSetting();
    }
    else if ( COMMONUI_IsInCoordinate(ZONE_SETPOINT_BUTTONUP_X, ZONE_SETPOINT_BUTTONUP_Y, EF_g_sIMAGES_ICON_ARROW_UP_EN_120X60_JPG.s32Width, EF_g_sIMAGES_ICON_ARROW_UP_EN_120X60_JPG.s32Height, s32TouchX, s32TouchY) )
    {
        bNeedRedraw = true;
        if (g_sMemblock.s2cGetStatusResp.stove_state.remote_temperature_setp.temp + 0.5f <= 40.0f)
            g_sMemblock.s2cGetStatusResp.stove_state.remote_temperature_setp.temp += 0.5f;
        g_sMemblock.isTemperatureSetPointChanged = true;
        ESPNOWCOMM_SendChangeSetting();
    }

    // ==================================
    // Configs fan speed
    if ( COMMONUI_IsInCoordinate(ZONE_FANSPEED_BUTTONDOWN_X, ZONE_FANSPEED_BUTTONDOWN_Y, EF_g_sIMAGES_ICON_ARROW_UP_EN_120X60_JPG.s32Width, EF_g_sIMAGES_ICON_ARROW_UP_EN_120X60_JPG.s32Height, s32TouchX, s32TouchY) )
    {
        bNeedRedraw = true;   
        if (g_sMemblock.s2cGetStatusResp.stove_state.fan_speed_set.curr - 1 >= 1)
            g_sMemblock.s2cGetStatusResp.stove_state.fan_speed_set.curr--;
        g_sMemblock.isFanSpeedSetPointChanged = true;
        ESPNOWCOMM_SendChangeSetting();
    }
    else if ( COMMONUI_IsInCoordinate(ZONE_FANSPEED_BUTTONUP_X, ZONE_FANSPEED_BUTTONUP_Y, EF_g_sIMAGES_ICON_ARROW_UP_EN_120X60_JPG.s32Width, EF_g_sIMAGES_ICON_ARROW_UP_EN_120X60_JPG.s32Height, s32TouchX, s32TouchY) )
    {
        bNeedRedraw = true;
        if (g_sMemblock.s2cGetStatusResp.stove_state.fan_speed_set.curr + 1 <= 4)
            g_sMemblock.s2cGetStatusResp.stove_state.fan_speed_set.curr++;
        g_sMemblock.isFanSpeedSetPointChanged = true;
        ESPNOWCOMM_SendChangeSetting();
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
        G_g_CanvasResult.setFreeFont(FF22);
        G_g_CanvasResult.setTextSize(2);
        sprintf(tmp, "Room %.1f C", tem);
        G_g_CanvasResult.drawCentreString(String(tmp), SCREEN_WIDTH / 3, 40, GFXFF);
            
        // -----------------------------------
        // Current set point
        G_g_CanvasResult.setFreeFont(FF23);
        G_g_CanvasResult.setTextSize(1);

        // S2C Get status response       
        if (g_sMemblock.has_s2cGetStatusResp && 
            g_sMemblock.s2cGetStatusResp.has_stove_state &&
            g_sMemblock.s2cGetStatusResp.stove_state.has_remote_temperature_setp)
        {
            const char* szTempUnitString = "";
            if (g_sMemblock.s2cGetStatusResp.stove_state.remote_temperature_setp.unit == SBI_iot_common_ETEMPERATUREUNIT_Celcius)
                szTempUnitString = "C";
            else if (g_sMemblock.s2cGetStatusResp.stove_state.remote_temperature_setp.unit == SBI_iot_common_ETEMPERATUREUNIT_Farenheit)
                szTempUnitString = "F";
            sprintf(tmp, "Set point: %.1f %s", g_sMemblock.s2cGetStatusResp.stove_state.remote_temperature_setp.temp, szTempUnitString);
        }
        else
        {
            sprintf(tmp, "Set point: ---"); 
        }
        G_g_CanvasResult.drawCentreString(String(tmp), SCREEN_WIDTH / 3, 100, GFXFF);
        
        // Arrows
        const EF_SFile* pSFileArrowUp = COMMONUI_GetBtnArrowUp(true);
        G_g_CanvasResult.drawJpg(pSFileArrowUp->pu8StartAddr, pSFileArrowUp->u32Length, ZONE_SETPOINT_BUTTONUP_X, ZONE_SETPOINT_BUTTONUP_Y);
        const EF_SFile* pSFileArrowDown = COMMONUI_GetBtnArrowDown(true);
        G_g_CanvasResult.drawJpg(pSFileArrowDown->pu8StartAddr, pSFileArrowDown->u32Length, ZONE_SETPOINT_BUTTONDOWN_X, ZONE_SETPOINT_BUTTONDOWN_Y);
    }
    // -----------------------------------
    // Room temperature separation line
    {
        const int32_t s32Left = 40; 
        G_g_CanvasResult.drawFastHLine(s32Left, 160, SCREEN_WIDTH - (s32Left*2), TFT_WHITE);
    }

    // -----------------------------------
    // System status
    {
        G_g_CanvasResult.setFreeFont(FF23);
        G_g_CanvasResult.setTextSize(1);

        const int32_t s32TopY_1 = 230; 
        G_g_CanvasResult.drawString("Etat: Comb. Eleve", 20, s32TopY_1);
        G_g_CanvasResult.drawString("650 C", 420, s32TopY_1);

        const EF_SFile* pSIconFlame80x80 = &EF_g_sFiles[EF_EFILE_ICON_FLAME_80X80_JPG];
        G_g_CanvasResult.drawJpg(pSIconFlame80x80->pu8StartAddr, pSIconFlame80x80->u32Length, 330, s32TopY_1-30);

        const int32_t s32TopY_2 = 300; 
        G_g_CanvasResult.drawString("Fumee: Faible", 20, s32TopY_2);
        G_g_CanvasResult.drawString("175", 420, s32TopY_2);

        const EF_SFile* pSIcon3Bars80x80 = &EF_g_sFiles[EF_EFILE_ICON_3BARS_80X80_JPG];
        G_g_CanvasResult.drawJpg(pSIcon3Bars80x80->pu8StartAddr, pSIcon3Bars80x80->u32Length, 330, s32TopY_2-30);
    }

    // System status separation line
    {
        const int32_t s32Left = 40; 
        G_g_CanvasResult.drawFastHLine(s32Left, 380, SCREEN_WIDTH - (s32Left*2), TFT_WHITE);
    }

    // -----------------------------------
    // Actions
    {
        G_g_CanvasResult.setFreeFont(FF23);
        G_g_CanvasResult.setTextSize(1);

        G_g_CanvasResult.drawCentreString("Action", SCREEN_WIDTH / 2, 380+20, GFXFF);

        G_g_CanvasResult.drawString("Ventilateur", 50, 380+20+80*1);
        const EF_SFile* pSIconFan72x72 = &EF_g_sFiles[EF_EFILE_ICON_FAN_72X72_JPG];
        G_g_CanvasResult.drawJpg(pSIconFan72x72->pu8StartAddr, pSIconFan72x72->u32Length, 330, 380+0+80*1);
        G_g_CanvasResult.drawString("min", 417, 380+20+80*1);

        G_g_CanvasResult.drawString("Distribution", 50, 380+20+80*2);
        const EF_SFile* pSIconDist72x72 = &EF_g_sFiles[EF_EFILE_ICON_DISTRIBUTION_72X72_JPG];
        G_g_CanvasResult.drawJpg(pSIconDist72x72->pu8StartAddr, pSIconDist72x72->u32Length, 330, 380+0+80*2);
        G_g_CanvasResult.drawString("max", 417, 380+20+80*2);

        G_g_CanvasResult.drawString("Boost", 50, 380+20+80*3);
        const EF_SFile* pSIconFireBoost72x72 = &EF_g_sFiles[EF_EFILE_ICON_FIREBOOST_72X72_JPG];
        G_g_CanvasResult.drawJpg(pSIconFireBoost72x72->pu8StartAddr, pSIconFireBoost72x72->u32Length, 330, 380+0+80*3);
        G_g_CanvasResult.drawString("55s", 417, 380+20+80*3);
    }

    // -----------------------------------
    // Fan speed.
    /*
    {
        G_g_CanvasResult.setFreeFont(FF19);
        G_g_CanvasResult.setTextSize(1);
        G_g_CanvasResult.drawCentreString("Fan speed", 40+EF_g_sIMAGES_ICON_ARROW_DOWN_EN_120X60_JPG.s32Width/2, ZONE_FANSPEED_START_Y+92, GFXFF);

        const uint8_t u8CurrentFanSpeed = (uint8_t)g_sMemblock.s2cGetStatusResp.stove_state.fan_speed_set.curr;

        if (pArgument->bIsUserModeActive)
        {
            const EF_SFile* pSFileArrowUp = COMMONUI_GetBtnArrowUp(u8CurrentFanSpeed != 4);
            G_g_CanvasResult.drawJpg(pSFileArrowUp->pu8StartAddr, pSFileArrowUp->u32Length, ZONE_FANSPEED_BUTTONUP_X, ZONE_FANSPEED_BUTTONUP_Y);

            const EF_SFile* pSFileArrowDown = COMMONUI_GetBtnArrowDown(u8CurrentFanSpeed != 1);
            G_g_CanvasResult.drawJpg(pSFileArrowDown->pu8StartAddr, pSFileArrowDown->u32Length, ZONE_FANSPEED_BUTTONDOWN_X, ZONE_FANSPEED_BUTTONDOWN_Y);
        }
        DrawAllBars(232, ZONE_FANSPEED_START_Y, u8CurrentFanSpeed);

        G_g_CanvasResult.drawRect(40, ZONE_FANSPEED_START_Y + 216, SCREEN_WIDTH-(40*2), 2, TFT_WHITE);
    }*/

    {
        // Message box
        const int32_t s32Width = SCREEN_WIDTH - (20 * 2);
        const int32_t s32Height = 210;
        const int32_t s32Left = (SCREEN_WIDTH / 2) - (s32Width / 2);
        const int32_t s32Top = (SCREEN_HEIGHT - s32Height - 20);
        G_g_CanvasResult.setTextSize(1);
        G_g_CanvasResult.setFreeFont(FF23);
        G_g_CanvasResult.drawRoundRect(s32Left, s32Top, s32Width, s32Height, 20, TFT_WHITE);
        
        G_g_CanvasResult.drawCentreString("MESSAGE", SCREEN_WIDTH/2, s32Top + 20, GFXFF);
        
        // 21 characters maximum
        G_g_CanvasResult.setFreeFont(FF19);
        const int32_t s32LineHeight = 40;
        const char* szLines[] = 
        {
            "ANTICONSTITUTIONNE#01",
            "ANTICONSTITUTIONNE#02",
            "ANTICONSTITUTIONNE#03"
        };
        for(int i = 0; i < sizeof(szLines)/sizeof(szLines[0]); i++)
        {
            G_g_CanvasResult.drawCentreString(szLines[i], SCREEN_WIDTH/2, s32Top + 80 + s32LineHeight*i, GFXFF);
        }
    }

    // -----------------------------------
    // Settings
    /*if (pArgument->bIsUserModeActive)
    {
        G_g_CanvasResult.drawJpg(pSFileSetting->pu8StartAddr, pSFileSetting->u32Length, ZONE_BTSETTING_START_X, ZONE_BTSETTING_START_Y);
    }

    G_g_CanvasResult.drawJpg(pSFileSBILogo->pu8StartAddr, pSFileSBILogo->u32Length, 16, 832);*/

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

static void OnDataReceived(COMMONUI_SContext* pContext)
{
    RedrawUI((COMMONUI_SContext*)pContext);
}