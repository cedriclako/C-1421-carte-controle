#include "ControlViewUI.hpp"
#include "Global.h"
#include "../MemBlock.h"
#include "../UIManager.hpp"
#include "espnowcomm.h"

#define TAG "ControlViewUI"

#define BAR_HEIGHT 192
#define BAR_WIDTH 48
#define BAR_MARGIN 16

#define ZONE_SETPOINT_START_Y (15)
#define ZONE_SETPOINT_BUTTONUP_X (360)
#define ZONE_SETPOINT_BUTTONUP_Y (ZONE_SETPOINT_START_Y+15)
#define ZONE_SETPOINT_BUTTONDOWN_X (360)
#define ZONE_SETPOINT_BUTTONDOWN_Y (ZONE_SETPOINT_START_Y+80)

#define ZONE_BTSETTING_START_X 344
#define ZONE_BTSETTING_START_Y 760

static void Init(COMMONUI_SContext* pContext);

static void Enter(COMMONUI_SContext* pContext);
static void Exit(COMMONUI_SContext* pContext);

static void Process(COMMONUI_SContext* pContext);

static void OnTouch(COMMONUI_SContext* pContext, int32_t s32X, int32_t s32Y);

static void OnDataReceived(COMMONUI_SContext* pContext);

static void RedrawUI(COMMONUI_SContext* pContext);

static void OnClick(COMMONUI_SContext* pContext, CONTROLVIEWUI_EBUTTONS eButton);

const COMMONUI_SConfig CONTROLVIEWUI_g_sConfig = 
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
    CONTROLVIEWUI_SHandle* pHandle = (CONTROLVIEWUI_SHandle*)pContext->pHandle;

    COMMONUI_Button_Init(&pHandle->sUIButtons[CONTROLVIEWUI_EBUTTONS_SetPointUp], &EF_g_sFiles[EF_EFILE_ICON_ARROW_UP_EN_120X60_JPG], ZONE_SETPOINT_BUTTONUP_X, ZONE_SETPOINT_BUTTONUP_Y);
    COMMONUI_Button_Init(&pHandle->sUIButtons[CONTROLVIEWUI_EBUTTONS_SetPointDn], &EF_g_sFiles[EF_EFILE_ICON_ARROW_DOWN_EN_120X60_JPG], ZONE_SETPOINT_BUTTONDOWN_X, ZONE_SETPOINT_BUTTONDOWN_Y);

    COMMONUI_Button_Init(&pHandle->sUIButtons[CONTROLVIEWUI_EBUTTONS_FanSpeedChange], &EF_g_sFiles[EF_EFILE_ICON_FAN_72X72_JPG], 330, 380+0+80*1);
    COMMONUI_Button_Init(&pHandle->sUIButtons[CONTROLVIEWUI_EBUTTONS_ActionDistribution], &EF_g_sFiles[EF_EFILE_ICON_DISTRIBUTION_72X72_JPG], 330, 380+0+80*2);
    COMMONUI_Button_Init(&pHandle->sUIButtons[CONTROLVIEWUI_EBUTTONS_ActionFireBoost], &EF_g_sFiles[EF_EFILE_ICON_FIREBOOST_72X72_JPG], 330, 380+0+80*3);
}

static void Enter(COMMONUI_SContext* pContext)
{   
    CONTROLVIEWUI_SHandle* pHandle = (CONTROLVIEWUI_SHandle*)pContext->pHandle;
    
    RedrawUI(pContext);
}

static void Exit(COMMONUI_SContext* pContext)
{

}

static void OnTouch(COMMONUI_SContext* pContext, int32_t s32TouchX, int32_t s32TouchY)
{
    const CONTROLVIEWUI_SArgument* pArgument = (const CONTROLVIEWUI_SArgument*)pContext->pvdArgument;
    if (!pArgument->bIsUserModeActive)
    {
        // Read-Only mode.
        return;
    }

    CONTROLVIEWUI_SHandle* pHandle = (CONTROLVIEWUI_SHandle*)pContext->pHandle;

    // ==================================
    // Set point
    // Configs fan speed
    for(int i = 0; i < CONTROLVIEWUI_EBUTTONS_Count; i++)
    {
        const CONTROLVIEWUI_EBUTTONS eButton = (CONTROLVIEWUI_EBUTTONS)i;
        if (COMMONUI_IsInCoordinate(&pHandle->sUIButtons[(int)eButton], s32TouchX, s32TouchY))
        {
            OnClick(pContext, eButton);
            break;
        }
    }

    ESP_LOGI(TAG, "Touch x: %d, y: %d", s32TouchX, s32TouchY);
}

static void OnClick(COMMONUI_SContext* pContext, CONTROLVIEWUI_EBUTTONS eButton)
{
    bool bNeedRedraw = false;

    switch( eButton )
    {
        case CONTROLVIEWUI_EBUTTONS_SetPointDn:
            bNeedRedraw = true;
            if (g_sMemblock.s2cGetStatusResp.stove_state.remote_temperature_setp.temp - 0.5f >= 5.0f)
                g_sMemblock.s2cGetStatusResp.stove_state.remote_temperature_setp.temp -= 0.5f;
            g_sMemblock.isTemperatureSetPointChanged = true;
            ESPNOWCOMM_SendChangeSetting();
            break;
        case CONTROLVIEWUI_EBUTTONS_SetPointUp:
            bNeedRedraw = true;
            if (g_sMemblock.s2cGetStatusResp.stove_state.remote_temperature_setp.temp + 0.5f <= 40.0f)
                g_sMemblock.s2cGetStatusResp.stove_state.remote_temperature_setp.temp += 0.5f;
            g_sMemblock.isTemperatureSetPointChanged = true;
            ESPNOWCOMM_SendChangeSetting();
            break;
        case CONTROLVIEWUI_EBUTTONS_FanSpeedChange:   
        {
            bNeedRedraw = true;
            
            int nFanSpeed = (int)g_sMemblock.s2cGetStatusResp.stove_state.fan_speed_set.curr;
            if (nFanSpeed + 1 >= SBI_iot_common_EFANSPEED_Count)
                nFanSpeed = 0;
            else
                nFanSpeed++;
            g_sMemblock.s2cGetStatusResp.stove_state.fan_speed_set.curr = (SBI_iot_common_EFANSPEED)nFanSpeed;

            g_sMemblock.isFanSpeedSetPointChanged = true;
            ESPNOWCOMM_SendChangeSetting();
            break;
        } 
        default:
            break;
    }

    if (bNeedRedraw)
        RedrawUI(pContext);
}

static void Process(COMMONUI_SContext* pContext)
{

}

static void RedrawUI(COMMONUI_SContext* pContext)
{
    //const CONTROLVIEWUI_SArgument* pArgument = (const CONTROLVIEWUI_SArgument*)pContext->pvdArgument;
    CONTROLVIEWUI_SHandle* pHandle = (CONTROLVIEWUI_SHandle*)pContext->pHandle;

    // rtc_time_t RTCtime;
    // rtc_date_t RTCDate;
    // M5.RTC.getTime(&RTCtime);
    // M5.RTC.getDate(&RTCDate);
    float tem = M5.SHT30.GetTemperature();
    //float hum = M5.SHT30.GetRelHumidity();
    char tmp[40+1];

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
        
        // Arrows - Up
        COMMONUI_Button_Draw(&pHandle->sUIButtons[CONTROLVIEWUI_EBUTTONS_SetPointUp]);

        // Arrows - Down
        COMMONUI_Button_Draw(&pHandle->sUIButtons[CONTROLVIEWUI_EBUTTONS_SetPointDn]);
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

        // Fan speed
        G_g_CanvasResult.drawString("Ventilateur", 50, 380+20+80*1);
        const COMMONUI_SButton* pBtFanChange = &pHandle->sUIButtons[CONTROLVIEWUI_EBUTTONS_FanSpeedChange];
        COMMONUI_Button_Draw(pBtFanChange);
        const char* szFanSpeedDesc = "";
        //static int32_t s_s32DescMaxWidth = 0;
        switch (g_sMemblock.s2cGetStatusResp.stove_state.fan_speed_set.curr)
        {
            case 0: szFanSpeedDesc = "off"; break;
            case 1: szFanSpeedDesc = "max"; break;
            case 2: szFanSpeedDesc = "min"; break;
            default: szFanSpeedDesc = "-err-"; break;
        }
        const int32_t s32Label1X = 417, s32Label1Y = 380+20+80*1;
        G_g_CanvasResult.fillRect(s32Label1X, pBtFanChange->sRect.s32Y, (SCREEN_WIDTH - s32Label1X), pBtFanChange->sRect.s32Height, TFT_BLACK);
        G_g_CanvasResult.drawString(szFanSpeedDesc, s32Label1X, s32Label1Y);

        // Distribution
        G_g_CanvasResult.drawString("Distribution", 50, 380+20+80*2);
        const COMMONUI_SButton* pBtActionDistribution = &pHandle->sUIButtons[CONTROLVIEWUI_EBUTTONS_ActionDistribution];
        COMMONUI_Button_Draw(pBtActionDistribution);
        const int32_t s32Label2X = 417, s32Label2Y = 380+20+80*2;
        //G_g_CanvasResult.fillRect(s32Label2X, pBtFanChange->sRect.s32Y, (SCREEN_WIDTH - s32Label1X), pBtFanChange->sRect.s32Height, TFT_BLACK);
        G_g_CanvasResult.drawString("max", s32Label2X, s32Label2Y);

        // Boost
        G_g_CanvasResult.drawString("Boost", 50, 380+20+80*3);
        const COMMONUI_SButton* pBtActionFireBoost = &pHandle->sUIButtons[CONTROLVIEWUI_EBUTTONS_ActionFireBoost];
        COMMONUI_Button_Draw(pBtActionFireBoost);
        G_g_CanvasResult.drawString("60s", 417, 380+20+80*3);
    }

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
            "TEST#02",
            "TEST#03"
        };
        for(int i = 0; i < sizeof(szLines)/sizeof(szLines[0]); i++)
        {
            G_g_CanvasResult.drawCentreString(szLines[i], SCREEN_WIDTH/2, s32Top + 80 + s32LineHeight*i, GFXFF);
        }
    }

    // Update screen
    G_g_CanvasResult.pushCanvas(0, 0, UPDATE_MODE_DU4);
}

static void OnDataReceived(COMMONUI_SContext* pContext)
{
    RedrawUI((COMMONUI_SContext*)pContext);
}