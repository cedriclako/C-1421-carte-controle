#include "OTAUploadESP32.h"
#include "esp_ota_ops.h"

#define TAG "OTAUploadESP32"

esp_err_t OTAUPLOADESP32_postotauploadESP32_handler(httpd_req_t *req)
{
    esp_ota_handle_t update_handle = 0;

    ESP_LOGI(TAG, "file_postotauploadESP32_handler / uri: %s", req->uri);

    CHECK_FOR_ACCESS_OR_RETURN();

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running)
    {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08"PRIx32", but running from offset 0x%08"PRIx32,
            (int32_t)configured->address, (int32_t)running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }

    ESP_LOGI(TAG, "Running partition type %"PRId32" subtype %"PRId32" (offset 0x%08"PRIx32")",
        (int32_t)running->type, (int32_t)running->subtype, (int32_t)running->address);

    const esp_partition_t* update_partition = esp_ota_get_next_update_partition(NULL);
    assert(update_partition != NULL);
    ESP_LOGI(TAG, "Writing to partition subtype %"PRId32" at offset 0x%"PRIx32,
        (int32_t)update_partition->subtype, (int32_t)update_partition->address);

    esp_err_t err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        goto ERROR;
    }

    int n = httpd_req_recv(req, (char*)g_u8Buffers, HTTPSERVER_BUFFERSIZE);
    int binary_file_length = 0;

    while(n > 0)
    {
        ESP_LOGI(TAG, "file_postotauploadESP32_handler / receiving: %d bytes", n);

        err = esp_ota_write( update_handle, (const void *)g_u8Buffers, n);
        if (err != ESP_OK)
        {
            goto ERROR;
        }
        binary_file_length += n;
        ESP_LOGD(TAG, "Written image length %d", binary_file_length);

        n = httpd_req_recv(req, (char*)g_u8Buffers, HTTPSERVER_BUFFERSIZE);
    }

    err = esp_ota_end(update_handle);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        } else {
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        }
        goto ERROR;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        goto ERROR;
    }

    ESP_LOGI(TAG, "OTA Completed !");
    ESP_LOGI(TAG, "Prepare to restart system!");

    esp_restart();

    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
    ERROR:
    if (update_handle != 0) {
        esp_ota_abort(update_handle);
    }

    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid image");
    return ESP_FAIL;
}

