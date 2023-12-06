#include "IoTBridge.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "fwconfig.h"
#include "settings.h"
#include "Main.h"
#include "log.h"

#define TAG "IoTBridge"

static void http_blobput_task(void *pvParameters);

void IOTBRIDGE_Init()
{

}

void IOTBRIDGE_Start()
{
    xTaskCreate(&http_blobput_task, FWCONFIG_IOTTASK_NAME, FWCONFIG_IOTTASK_STACKSIZE, NULL, FWCONFIG_IOTTASK_PRIORITY, NULL);
}

static void http_blobput_task(void *pvParameters)
{
    struct tm timeinfo;
    time_t now;
    char arrMin[3];
    char arrHourMin[5];
    char arrYearMonthDay[9];
    char arrUrl[1024];
    size_t n = SETTINGS_ESPNOWREMOTEMAC_LEN;
    char szMacAddr[SETTINGS_ESPNOWREMOTEMAC_LEN];
    memset(szMacAddr, '\0', sizeof(szMacAddr));
    
    NVSJSON_GetValueString(&g_sSettingHandle, SETTINGS_EENTRY_ESPNowRemoteMac, (char*)szMacAddr, &n);
    removeChar(szMacAddr, ':');
    vTaskDelay((15000) / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "Starting HTTP IoT task");

    for(;;) 
    {
        time(&now);
        localtime_r(&now, &timeinfo);
        memset(arrMin, '\0', sizeof(arrMin));
        memset(arrHourMin, '\0', sizeof(arrHourMin));
        memset(arrYearMonthDay, '\0', sizeof(arrYearMonthDay));
        memset(arrUrl, '\0', sizeof(arrUrl));
        strftime(arrMin, sizeof(arrMin), "%M", &timeinfo);
        strftime(arrHourMin, sizeof(arrHourMin), "%H%M", &timeinfo);
        strftime(arrYearMonthDay, sizeof(arrYearMonthDay), "%Y%m%d", &timeinfo);
       
        strcat(arrUrl, "https://sbistoragecount.blob.core.windows.net/self-regulated-stove/debug_");
        strcat(arrUrl, szMacAddr);
        strcat(arrUrl, "_");
        strcat(arrUrl, arrYearMonthDay);
        strcat(arrUrl, ".txt?sp=racwdli&st=2023-11-30T20:55:42Z&se=2025-12-01T04:55:42Z&spr=https&sv=2022-11-02&sr=c&sig=W60mLCkClwkRYR7LLZOPEk8JUpshSWJqLLx%2Bog9J0a8%3D");
        if((MAIN_GetIsWiFiConnected()) && ((atoi(arrMin) % 25) == 9)/*&& (strcmp(arrHourMin, "2359"))*/)
        {
            char *post_data = NULL;
            FILE* recordFileRead = NULL;
            char szFilename[96+1];
            char debugStringRead[640];
            char contentDispositionHeader[96];
            memset(contentDispositionHeader, '\0', sizeof(contentDispositionHeader));
            snprintf(contentDispositionHeader, sizeof(contentDispositionHeader), "attachment; filename=\"debug_%s_%s.txt\"", szMacAddr, arrYearMonthDay);
            memset(szFilename, '\0', sizeof(szFilename));
            snprintf(szFilename, sizeof(szFilename), FWCONFIG_SDCARD_ROOTPATH"/debug_%s.txt", arrYearMonthDay);
            ESP_LOGI(TAG, "szFilename: %s", szFilename);
            recordFileRead = fopen(szFilename, "r");

            ESP_LOGI(TAG, "Upload HTTP IoT task, filename: %s", szFilename);

            if (recordFileRead == NULL)
            {
                ESP_LOGE(TAG, "Failed to open file for reading");
                fclose(recordFileRead);
            }
            else
            {
                ESP_LOGI(TAG, "read file on SDCard, filename: '%s'", szFilename);
                post_data = (char*) malloc( 650 * 6 * sizeof(char));
                memset(post_data, '\0', sizeof(*post_data));
                // Get file size
                fseek(recordFileRead, 0, SEEK_END);
                uint64_t recordFileSize = ftell(recordFileRead);
                fseek(recordFileRead, 0, SEEK_SET);
                ESP_LOGI(TAG, "File size: %lld bytes", recordFileSize);
                char recordHeaderSize[16];
                itoa(recordFileSize,recordHeaderSize,10);
                esp_http_client_config_t config = {
                    .url = arrUrl,
                    .method = HTTP_METHOD_PUT,
                };
                esp_http_client_handle_t client = esp_http_client_init(&config);
                
                // PUT
                esp_http_client_set_header(client, "Content-Type", "text/plain; charset=UTF-8");
                esp_http_client_set_header(client, "x-ms-blob-type", "BlockBlob");
                esp_http_client_set_header(client, "Content-Disposition", contentDispositionHeader);
                esp_http_client_set_header(client, "Content-Length", recordHeaderSize);
         
                esp_err_t err = esp_http_client_open(client, recordFileSize);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
                    esp_http_client_cleanup(client);
                    return;
                } 
                int countPostData = 0;
                while(fgets(debugStringRead, sizeof(debugStringRead), recordFileRead) != NULL)
                { 
                    strcat(post_data, debugStringRead);
                    countPostData++;
                    if(countPostData % 6 == 0)
                    {
                        esp_http_client_write(client, &post_data[0], strlen(post_data));
                        countPostData = 0;
                        memset(post_data, '\0', sizeof(*post_data));
                    }
                    
                    memset(debugStringRead, '\0', sizeof(debugStringRead));
                    vTaskDelay((10) / portTICK_PERIOD_MS);
                    
                }  
                esp_http_client_write(client, &post_data[0], strlen(post_data));
                countPostData = 0;
                memset(post_data, '\0', sizeof(*post_data));
                esp_http_client_close(client);
                esp_http_client_cleanup(client);
            }
            
            if (recordFileRead != NULL) {
                fclose(recordFileRead);
            }
            free(post_data);
        }
        vTaskDelay((30000) / portTICK_PERIOD_MS); // 30 secondes
    }
 
    vTaskDelete(NULL);
}