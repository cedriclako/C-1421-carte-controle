#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"
#include "fwconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_log.h"

#define TAG "LOG"

static FILE* m_currRecordFile = NULL;
static TickType_t m_xLastWrite = 0;

void LOG_Init()
{
    m_xLastWrite = xTaskGetTickCount();

    const char* szFilename = FWCONFIG_SDCARD_ROOTPATH"/test.txt";

    ESP_LOGI(TAG, "Creating file on SDCard, filename: '%s'", szFilename);

    m_currRecordFile = fopen(szFilename, "a");
    if (m_currRecordFile == NULL)
    {
        goto ERROR;
    }
    return;
    ERROR:
    ESP_LOGE(TAG, "Failed to open file for writing");
    if (m_currRecordFile != NULL)
        fclose(m_currRecordFile);
    m_currRecordFile = NULL;
}

void LOG_Handler()
{
    // Nothing to do if the file is not open
    if (m_currRecordFile == NULL)
        return;

    const uint32_t u32DelayMS = pdTICKS_TO_MS(xTaskGetTickCount() - m_xLastWrite);
    /*if (u32DelayMS >= 1000)
    {
        m_xLastWrite = xTaskGetTickCount();

        const char* test = "test\r\n";
        fwrite(test, strlen(test), 1, m_currRecordFile);
    }*/
}