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
static TickType_t m_xLastDebugStringTicks = 0;
static uint32_t m_u32AutoSaveDelayS = 0;

static void SaveDebugStringIntoFile();

void LOG_Init()
{
    m_xLastWrite = xTaskGetTickCount();
    m_u32AutoSaveDelayS = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_AutoSaveDebugString);
    
    ESP_LOGI(TAG, "Log data automatically every %" PRIu32 " s", m_u32AutoSaveDelayS);
}

void LOG_Handler()
{
    const uint32_t u32DelayMS = pdTICKS_TO_MS(xTaskGetTickCount() - m_xLastWrite);

    /* Record the debug string every minutes */
    if (u32DelayMS >= m_u32AutoSaveDelayS * 1000)
    {
        m_xLastWrite = xTaskGetTickCount();

        SaveDebugStringIntoFile();
    }
}

static void SaveDebugStringIntoFile()
{
    bool hasTaken = false;
    FILE* currRecordFile = NULL;

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

    hasTaken = STOVEMB_Take(10);
    if (!hasTaken)
    {
        goto ERROR;
    }

    //ttDebugLastTicks
    const STOVEMB_SMemBlock* pMemBlock = STOVEMB_GetMemBlockRO();
    const int debugJSONLen = strlen(pMemBlock->szDebugJSONString);
    const bool bIsDebugStringValid = 
        debugJSONLen > 0 &&
        (pMemBlock->ttDebugJSONLastTicks != m_xLastDebugStringTicks);

    char szTimeStamp[60];
    snprintf(szTimeStamp, sizeof(szTimeStamp), 
        "%04d-%02d-%02d_%02d:%02d:%02d",
        /*0*/(int)info->tm_year + 1900,
        /*1*/(int)info->tm_mon + 1,
        /*2*/(int)info->tm_mday,
        
        /*3*/(int)info->tm_hour,
        /*4*/(int)info->tm_min,
        /*5*/(int)info->tm_sec);

    // Print timestamp
    fwrite(szTimeStamp, strlen(szTimeStamp), sizeof(uint8_t), currRecordFile);
    fwrite(" ", 1, sizeof(uint8_t), currRecordFile);

    if (bIsDebugStringValid)
    {
        m_xLastDebugStringTicks = pMemBlock->ttDebugJSONLastTicks;
        // Debug JSON string
        fwrite(pMemBlock->szDebugJSONString, debugJSONLen, sizeof(uint8_t), currRecordFile);
        ESP_LOGI(TAG, "Saved on SDCard");
    }
    else
    {
        ESP_LOGW(TAG, "Saved on SDCard, but the debug string is invalid");
    }
    // Write nothing ... to indicate we have nothing to write. Could be an error
    fwrite("\r\n", 2, sizeof(uint8_t), currRecordFile);
    
    goto END;
    ERROR:
    ESP_LOGE(TAG, "Failed to open file for writing");
    END:
    if (currRecordFile != NULL) {
        fclose(currRecordFile);
    }
    if (hasTaken) {
        STOVEMB_Give();   
    }
}