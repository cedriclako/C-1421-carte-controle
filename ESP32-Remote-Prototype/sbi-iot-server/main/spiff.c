#include "spiff.h"
#include "esp_spiffs.h"
#include "esp_log.h"

#define TAG "SPIFF"

static bool m_bIsInitialized = false;

void SPIFF_Init(void)
{
    ESP_LOGI(TAG, "%s", "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf =
	{
		.base_path = FWCONFIG_SPIFF_ROOTPATH,
		.partition_label = FWCONFIG_SPIFF_PARTITION,
		.max_files = 32,
		.format_if_mount_failed = true
	};

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "%s", "SPIFF initialization failed");
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "%s", "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "%s", "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    // Formatting SPIFFS - Use only for debugging
    /*if (esp_spiffs_format(NULL) != ESP_OK)
    {
        ESP_LOGE(TAG, "%s", "Failed to format SPIFFS");
        return;
    }*/

    size_t total, used;
    if (esp_spiffs_info(FWCONFIG_SPIFF_PARTITION, &total, &used) == ESP_OK)
    {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    else
    {
        ESP_LOGW(TAG, "SPIFF unable to get SPIFF info");
    }

    m_bIsInitialized = true;
}

bool SPIFF_GetIsInitialized()
{
    return m_bIsInitialized;
}

bool SPIFF_GetFileSystemInfo(size_t* pTotalBytes, size_t* pUsedBytes)
{
    if (!SPIFF_GetIsInitialized())
        return false;

    if (esp_spiffs_info(FWCONFIG_SPIFF_PARTITION, pTotalBytes, pUsedBytes) != ESP_OK)
        return false;
    return true;
}
