#include <M5EPD.h>
#include "Free_Fonts.h"
#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_sleep.h"

#include "espnowcomm.h"
#include "assets/EmbeddedFiles.h"

#define TAG "main"

#define SCREEN_WIDTH 540
#define SCREEN_HEIGHT 960

typedef struct 
{
    struct
    {
        uint32_t u32WakeUpCount;
        uint8_t u8ESPNowChannel;

        uint8_t u8NoDataCount;
    } sData;
    uint8_t u8Checksum;
} SDeepSleepState;

static RTC_DATA_ATTR SDeepSleepState m_sDeepSleepState;

static uint8_t CalculateDeepSleepDataChecksum(const SDeepSleepState* pDeepSleepState);
static void UpdateDeepSleepDataChecksum(SDeepSleepState* pDeepSleepState);
static bool CheckDeepSleepDataChecksum(SDeepSleepState* pDeepSleepState);

static void ENS2CGetStatusRespCallback(const SBI_iot_S2CGetStatusResp* pMsg);
static void ENChannelFoundCallback(uint8_t u8Channel);

static void DrawAllBars(int32_t s32X, int32_t s32Y, uint32_t u32Bar);

static void UpdateScreen();

static bool m_bDataReceived = false;
static TickType_t m_ttProcTimeoutTicks = xTaskGetTickCount();

static M5EPD_Canvas m_CanvasResult(&M5.EPD);

void setup()
{
    // Check if RTC memory is already initialized.
    if (!CheckDeepSleepDataChecksum(&m_sDeepSleepState))
    {
        ESP_LOGI(TAG, "Init deep sleep data conservation");
        // Initialize deep sleep data structure
        memset(&m_sDeepSleepState.sData, 0, sizeof(m_sDeepSleepState.sData));
    }

    m_sDeepSleepState.sData.u32WakeUpCount++;
    UpdateDeepSleepDataChecksum(&m_sDeepSleepState);
    ESP_LOGI(TAG, "Booted: %d ms, boot count: %d, esp-now channel: %d", 
        (int)(esp_timer_get_time() / 1000), 
        m_sDeepSleepState.sData.u32WakeUpCount,
        m_sDeepSleepState.sData.u8ESPNowChannel);

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_LOGI(TAG, "Initializing M5");
    M5.begin(true, false, true, true);
    ESP_LOGI(TAG, "Initializing SHT30");
    M5.SHT30.Begin();
    ESP_LOGI(TAG, "Initializing RTC");
    M5.RTC.begin();

    M5.TP.SetRotation(90);
    M5.EPD.SetRotation(90);

    ESP_LOGI(TAG, "CreateCanvas");
    m_CanvasResult.createCanvas(SCREEN_WIDTH, SCREEN_HEIGHT);


    M5.update();
    if (M5.BtnP.isPressed())
    {
        ESP_LOGI(TAG, "Started using the button");
        M5.EPD.Clear(true);
        m_CanvasResult.setTextSize(2);
        m_CanvasResult.setFreeFont(FF19);
        m_CanvasResult.drawString("Powering on ...", 0, 850);
        m_CanvasResult.pushCanvas(0, 0, UPDATE_MODE_A2);
        m_CanvasResult.fillCanvas(0);
    }

    ESP_LOGI(TAG, "Init netif");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Restore last channel
    ESPNOWCOMM_Init(m_sDeepSleepState.sData.u8ESPNowChannel);

    ESPNOWCOMM_SetChannelFoundCallback(ENChannelFoundCallback);
    ESPNOWCOMM_SetS2CGetStatusRespCallback(ENS2CGetStatusRespCallback);

    ESP_LOGI(TAG, "FreeRTOS period: %d ms", portTICK_PERIOD_MS);
}

void loop()
{
    ESPNOWCOMM_Handler();
    
    M5.SHT30.UpdateData();

    // 5s maximum, after that we go to sleep again.
    if ((xTaskGetTickCount() - m_ttProcTimeoutTicks) > pdMS_TO_TICKS(10000) || m_bDataReceived)
    {
        // Timeout
        if (!m_bDataReceived)
        {
            ESP_LOGE(TAG, "No communication, reset ESPNowChannel, next time it will scan");
            m_sDeepSleepState.sData.u8ESPNowChannel = 0;
            UpdateDeepSleepDataChecksum(&m_sDeepSleepState);
        }

        // Update screen
        UpdateScreen();

        ESP_LOGI(TAG, "Time to go to sleep, good night. wk.count: %d, time: %d", 
            m_sDeepSleepState.sData.u32WakeUpCount, 
            (int)(esp_timer_get_time() / 1000));
        
        // Shutdown make deep sleep useless, except when the USB port keep the device alive
        // we can use deep sleep to simulate power loss.
        M5.shutdown(10);
        TickType_t ttTicks = xTaskGetTickCount();
        while( (xTaskGetTickCount() - ttTicks) <= pdMS_TO_TICKS(10*1000))
        {       
            M5.update();
            if (M5.BtnP.isPressed())
                break;
            vTaskDelay(1);
        }
        esp_restart();
        //esp_sleep_enable_timer_wakeup(10 * 1000000);
        //esp_deep_sleep_start();
    }
}

static void UpdateScreen()
{
    ESP_LOGI(TAG, "Clear EPD");
    M5.EPD.Clear(true);
    
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

    // 
    const EF_SFile* pSFileArrowUp = &EF_g_sFiles[EF_EFILE_ICON_ARROW_UP_120X60_JPG];
    const EF_SFile* pSFileArrowDown = &EF_g_sFiles[EF_EFILE_ICON_ARROW_DOWN_120X60_JPG];
    const EF_SFile* pSFileSetting = &EF_g_sFiles[EF_EFILE_ICON_SETTING_160X160_JPG];
    const EF_SFile* pSFileSBILogo = &EF_g_sFiles[EF_EFILE_ICON_SBI_LOGO_152X112_JPG];

    // -----------------------------------
    // Current room temperature
    {
        const int32_t s32CurrentTempY = 0;

        m_CanvasResult.setFreeFont(FF18);
        m_CanvasResult.setTextSize(2);
        m_CanvasResult.drawCentreString("Room", 40+120/2, s32CurrentTempY+96, GFXFF);

        m_CanvasResult.setFreeFont(FSSB18);
        m_CanvasResult.setTextSize(3);
        m_CanvasResult.drawString(String(temStr), 232, s32CurrentTempY+72);
        m_CanvasResult.setTextSize(1);
        m_CanvasResult.drawString("C", 440, s32CurrentTempY+72);
        
        m_CanvasResult.drawRect(40, s32CurrentTempY + 216, SCREEN_WIDTH-(40*2), 2, TFT_WHITE);
    }

    // -----------------------------------
    // Current set point
    {
        const int32_t s32CurrentTempY = 250;

        m_CanvasResult.setFreeFont(FF19);
        m_CanvasResult.setTextSize(1);
        m_CanvasResult.drawCentreString("Setpoint", 40+120/2, s32CurrentTempY+92, GFXFF);
        m_CanvasResult.drawJpg(pSFileArrowUp->pu8StartAddr, pSFileArrowUp->u32Length, 40, s32CurrentTempY+16);
        m_CanvasResult.drawJpg(pSFileArrowDown->pu8StartAddr, pSFileArrowDown->u32Length, 40, s32CurrentTempY+136);

        m_CanvasResult.setFreeFont(FSSB18);
        m_CanvasResult.setTextSize(3);
        m_CanvasResult.drawString(String(temStr), 232, s32CurrentTempY+72);
        m_CanvasResult.setTextSize(1);
        m_CanvasResult.drawString("C", 440, s32CurrentTempY+72);
        
        m_CanvasResult.drawRect(40, s32CurrentTempY + 216, SCREEN_WIDTH-(40*2), 2, TFT_WHITE);
    }

    // -----------------------------------
    // Current temperature
    {
        const int32_t s32CurrentTempY = 500;

        m_CanvasResult.setFreeFont(FF19);
        m_CanvasResult.setTextSize(1);
        m_CanvasResult.drawCentreString("Fan speed", 40+120/2, s32CurrentTempY+92, GFXFF);
        m_CanvasResult.drawJpg(pSFileArrowUp->pu8StartAddr, pSFileArrowUp->u32Length, 40, s32CurrentTempY+16);
        m_CanvasResult.drawJpg(pSFileArrowDown->pu8StartAddr, pSFileArrowDown->u32Length, 40, s32CurrentTempY+136);

        DrawAllBars(232, s32CurrentTempY, 3);

        m_CanvasResult.drawRect(40, s32CurrentTempY + 216, SCREEN_WIDTH-(40*2), 2, TFT_WHITE);
    }

    m_CanvasResult.drawJpg(pSFileSetting->pu8StartAddr, pSFileSetting->u32Length, 344, 760);
    m_CanvasResult.drawJpg(pSFileSBILogo->pu8StartAddr, pSFileSBILogo->u32Length, 16, 832);

    m_CanvasResult.pushCanvas(0, 0, UPDATE_MODE_DU);
    vTaskDelay(pdMS_TO_TICKS(300));
}

static void DrawAllBars(int32_t s32X, int32_t s32Y, uint32_t u32Bar)
{
    #define BAR_HEIGHT 192
    #define BAR_WIDTH 48
    #define BAR_MARGIN 16

    #define BAR_TOP(x) ((BAR_HEIGHT/32)*(32-x))
    #define DrawAllBars(_index, _level, _isActive) _isActive ? m_CanvasResult.fillRect(s32X+(BAR_WIDTH+BAR_MARGIN)*_index, s32Y + BAR_TOP(_level), BAR_WIDTH, BAR_HEIGHT - BAR_TOP(_level), TFT_WHITE) : m_CanvasResult.drawRect(s32X+(BAR_WIDTH+BAR_MARGIN)*_index, s32Y + BAR_TOP(_level), BAR_WIDTH, BAR_HEIGHT - BAR_TOP(_level), TFT_WHITE)
 
    /* 1/4 */DrawAllBars(0, 12, (u32Bar >= 1));
    /* 2/4 */DrawAllBars(1, 20, (u32Bar >= 2));
    /* 3/4 */DrawAllBars(2, 28, (u32Bar >= 3));
    /* 4/4 */DrawAllBars(3, 32, (u32Bar >= 4));
}

extern "C" void app_main()
{
    ESP_LOGI(TAG, "initArduino");
    initArduino();

    ESP_LOGI(TAG, "init setup");
    setup();

    ESP_LOGI(TAG, "main loop ...");
    while(1)
    {
        loop();
        vTaskDelay(1);
    }
}

static void ENChannelFoundCallback(uint8_t u8Channel)
{
    // Keep last known channel
    m_sDeepSleepState.sData.u8ESPNowChannel = u8Channel;
    UpdateDeepSleepDataChecksum(&m_sDeepSleepState);

    ESP_LOGI(TAG, "Right channel found!, channel: %d", u8Channel);
}

static void ENS2CGetStatusRespCallback(const SBI_iot_S2CGetStatusResp* pMsg)
{
    if (pMsg->has_stove_state && pMsg->stove_state.has_fan_speed_set && pMsg->stove_state.has_fan_speed_boundary)
    {
        ESP_LOGI(TAG, "temp. sp: %f, fanmode: %d, fanspeed: %d [%d-%d]", 
            pMsg->stove_state.remote_temperature_set.tempC_sp,
            (int)pMsg->stove_state.fan_speed_set.is_automatic,
            (int)pMsg->stove_state.fan_speed_set.curr,
            
            (int)pMsg->stove_state.fan_speed_boundary.min,
            (int)pMsg->stove_state.fan_speed_boundary.max);
    }
    else
    {
        ESP_LOGE(TAG, "GetStatus received but no stove_state");
    }
   m_bDataReceived = true;
}

static uint8_t CalculateDeepSleepDataChecksum(const SDeepSleepState* pDeepSleepState)
{
    uint8_t u8Checksum = 0;
    for(int i = 0; i < sizeof(pDeepSleepState->sData); i++)
    {
        const uint8_t u8 = *((uint8_t*)&pDeepSleepState->sData + i);
        u8Checksum += u8;
    }
    return ~u8Checksum;
}

static void UpdateDeepSleepDataChecksum(SDeepSleepState* pDeepSleepState)
{
    pDeepSleepState->u8Checksum = CalculateDeepSleepDataChecksum(pDeepSleepState);
}

static bool CheckDeepSleepDataChecksum(SDeepSleepState* pDeepSleepState)
{
    const uint8_t u8Checksum  = CalculateDeepSleepDataChecksum(pDeepSleepState);

    ESP_LOGI(TAG, "Curr checksum: %d, last checksum: %d", u8Checksum, pDeepSleepState->u8Checksum);
    return u8Checksum == pDeepSleepState->u8Checksum;
}