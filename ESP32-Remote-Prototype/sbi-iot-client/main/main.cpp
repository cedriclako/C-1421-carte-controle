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

#include "MemBlock.h"
#include "espnowcomm.h"
#include "SleepData.h"
#include "Global.h"
#include "UIManager.h"
#include "assets/EmbeddedFiles.h"

#define TAG "main"

static void ENS2CGetStatusRespCallback(const SBI_iot_S2CGetStatusResp* pMsg);
static void ENChannelFoundCallback(uint8_t u8Channel);

static bool m_bDataReceived = false;
static bool m_isUserModeActive = false;

static uint16_t m_u16Finger0X = 0;
static uint16_t m_u16Finger0Y = 0;

static TickType_t m_ttProcTimeoutTicks = xTaskGetTickCount();

// Last record
static SLEEPDATA_URecord m_uLastRecord = { .sData = { .u8LastChannel = 0 } };

void setup()
{
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

    M5.SHT30.UpdateData();

    M5.TP.SetRotation(90);
    M5.EPD.SetRotation(90);

    SLEEPDATA_Init();

    UIMANAGER_Init();

    M5.update();
    if (M5.BtnP.isPressed())
    {
        m_isUserModeActive = true;
        UIMANAGER_SwitchTo(ESCREEN_HomeUsermode);
    }
    else
    {
        UIMANAGER_SwitchTo(ESCREEN_HomeReadOnly);
    }

    ESP_LOGI(TAG, "Init netif");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Restore last channel
    if (SLEEPDATA_ReadLastRecord(&m_uLastRecord))
    {
        ESP_LOGI(TAG, "Last channel on record: %d", m_uLastRecord.sData.u8LastChannel);

        if (m_uLastRecord.sData.u8LastChannel > 11)
            m_uLastRecord.sData.u8LastChannel = 0;
    }

    ESPNOWCOMM_Init(m_uLastRecord.sData.u8LastChannel);

    ESPNOWCOMM_SetChannelFoundCallback(ENChannelFoundCallback);
    ESPNOWCOMM_SetS2CGetStatusRespCallback(ENS2CGetStatusRespCallback);

    ESP_LOGI(TAG, "FreeRTOS period: %d ms", portTICK_PERIOD_MS);
}

void loop()
{     
    M5.update();
    M5.SHT30.UpdateData();

    // Update status memory block ...
    g_sMemblock.sRemoteState.temperatureC_curr = M5.SHT30.GetTemperature();
    g_sMemblock.has_sRemoteState = true;

    ESPNOWCOMM_Handler();

    UIMANAGER_Process();

    if (m_isUserModeActive)
    {
        if (M5.TP.available()) 
        {
            if (!M5.TP.isFingerUp()) 
            {
                M5.TP.update();
                const tp_finger_t sFinger = M5.TP.readFinger(0);
                //M5.TP.flush();

                if (m_u16Finger0X != sFinger.x || m_u16Finger0Y != sFinger.y)
                {
                    // Reset timeout
                    m_ttProcTimeoutTicks = xTaskGetTickCount();

                    UIMANAGER_OnTouch(sFinger.x, sFinger.y);

                    m_u16Finger0X = sFinger.x;
                    m_u16Finger0Y = sFinger.y;
                }
            }
        }
    }
    else
    {
        // Switch to edit mode
        if (M5.BtnP.isPressed())
        {
            m_isUserModeActive = true;
            UIMANAGER_SwitchTo(ESCREEN_HomeUsermode);
        }
    }

    // 10s maximum, after that we go to sleep again
    const bool bIsExpired = (!m_isUserModeActive && ( (xTaskGetTickCount() - m_ttProcTimeoutTicks) > pdMS_TO_TICKS(10*1000) || m_bDataReceived )) ||
                             (m_isUserModeActive && (xTaskGetTickCount() - m_ttProcTimeoutTicks) > pdMS_TO_TICKS(20*1000));

    if (bIsExpired)
    {
        // Timeout
        if (!m_bDataReceived)
        {
            m_uLastRecord.sData.u8LastChannel = 0;
            ESP_LOGE(TAG, "No communication, reset ESPNowChannel, next time it will scan");
        }

        // Update screen
        if (m_isUserModeActive)
        {
            ESP_LOGI(TAG, "User is done with change, time to switch to readonly mode and sleep");
            m_isUserModeActive = false;
            UIMANAGER_SwitchTo(ESCREEN_HomeReadOnly);
        }

        vTaskDelay(pdMS_TO_TICKS(200));

        // Keep last channel into flash memory
        SLEEPDATA_WriteRecord(&m_uLastRecord);

        ESP_LOGI(TAG, "Time to go to sleep, good night. time: %d", 
            (int)(esp_timer_get_time() / 1000));
        
        // Shutdown make deep sleep useless, except when the USB port keep the device alive
        // we can use deep sleep to simulate power loss.
        const int32_t s32SleepTimeS = 60;
        M5.shutdown(s32SleepTimeS);
        TickType_t ttTicks = xTaskGetTickCount();
        while( (xTaskGetTickCount() - ttTicks) <= pdMS_TO_TICKS(s32SleepTimeS*1000))
        {       
            M5.update();
            if (M5.BtnP.isPressed())
                break;
            vTaskDelay(1);
        }
        esp_restart();
    }
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
    ESP_LOGI(TAG, "Right channel found!, channel: %d", u8Channel);
    m_uLastRecord.sData.u8LastChannel = u8Channel;
}

static void ENS2CGetStatusRespCallback(const SBI_iot_S2CGetStatusResp* pMsg)
{
    if (pMsg->has_stove_state && 
        pMsg->stove_state.has_fan_speed_set && 
        pMsg->stove_state.has_fan_speed_boundary && 
        pMsg->stove_state.has_remote_temperature_setp)
    {
        // Status received ...
        g_sMemblock.has_s2cGetStatusResp = true;
        memcpy(&g_sMemblock.s2cGetStatusResp, pMsg, sizeof(SBI_iot_S2CGetStatusResp));

        UIMANAGER_OnDataReceived();

        ESP_LOGI(TAG, "temp. sp: %f %s, fanmode: %s, fanspeed: %d [%d-%d]", 
            pMsg->stove_state.remote_temperature_setp.temp,
            ((pMsg->stove_state.remote_temperature_setp.unit == SBI_iot_common_ETEMPERATUREUNIT_Farenheit) ? "F" : "C"),

            (pMsg->stove_state.fan_speed_set.is_automatic ? "AUTO" : "MANUAL"),

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

