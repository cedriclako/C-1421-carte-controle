#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "log.h"
#include "fwconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "uartbridge/stovemb.h"
#include "uartbridge/uartbridge.h"
#include "nvsjson.h"
#include "settings.h"

#define TAG "LOG"

static TickType_t m_xLastWrite = 0;
static uint32_t m_u32AutoSaveDelayS = 0;

void LOG_Init()
{
    m_xLastWrite = xTaskGetTickCount();
    m_u32AutoSaveDelayS = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_AutoSaveDebugString);
    
    ESP_LOGI(TAG, "Log data automatically every %" PRIu32 " s", m_u32AutoSaveDelayS);
}

void LOG_Handler()
{
    const uint32_t u32DelayMS = pdTICKS_TO_MS(xTaskGetTickCount() - m_xLastWrite);
    FILE* currRecordFile = NULL;

    /* Record the debug string every minutes */
    if (u32DelayMS >= m_u32AutoSaveDelayS * 1000)
    {
        m_xLastWrite = xTaskGetTickCount();
            
        char szFilename[96+1];

        // Generate the timestamp using local time (we get it using the NTP protocol)
        time_t rawtime;
        time( &rawtime );
        struct tm* info = localtime( &rawtime );

        snprintf(szFilename, sizeof(szFilename), 
            FWCONFIG_SDCARD_ROOTPATH"/debug_%04d%02d%02d.txt",
            /*0*/(int)info->tm_year + 1900,
            /*1*/(int)info->tm_mon + 1,
            /*2*/(int)info->tm_mday);

        ESP_LOGI(TAG, "Creating file on SDCard, filename: '%s'", szFilename);

        currRecordFile = fopen(szFilename, "a");
        if (currRecordFile == NULL)
        {
            ESP_LOGE(TAG, "Unable to create file to save the debug string");
            goto ERROR;
        }

        const STOVEMB_SMemBlock* pMemBlock = STOVEMB_GetMemBlockRO();
        const int n = strlen(pMemBlock->szDebugJSONString);
        if (n > 0)
        {
            char szTimeStamp[60];
            snprintf(szTimeStamp, sizeof(szTimeStamp), 
                "%04d-%02d-%02d_%02d:%02d:%02d",
                /*0*/(int)info->tm_year + 1900,
                /*1*/(int)info->tm_mon + 1,
                /*2*/(int)info->tm_mday,
                
                /*3*/(int)info->tm_hour,
                /*4*/(int)info->tm_min,
                /*5*/(int)info->tm_sec);

            // Print timesamp
            fwrite(szTimeStamp, strlen(szTimeStamp), sizeof(uint8_t), currRecordFile);
            fwrite(" ", 1, sizeof(uint8_t), currRecordFile);
            // Debug JSON string
            fwrite(pMemBlock->szDebugJSONString, n, sizeof(uint8_t), currRecordFile);
            fwrite("\r\n", 2, sizeof(uint8_t), currRecordFile);
            ESP_LOGI(TAG, "Saved");
        }
        goto END;
        ERROR:
        ESP_LOGE(TAG, "Failed to open file for writing");
        END:
        if (currRecordFile != NULL) {
            fclose(currRecordFile);
        }
    }
}