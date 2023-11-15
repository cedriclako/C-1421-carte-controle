#include "OTACheck.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_ota_ops.h"
#include "fwconfig.h"
#include "cJSON.h"

#define TAG "OTACheck"

#define OTACHECK_TASK_PRIORITY FWCONFIG_OTACHECKTASK_PRIORITY
#define OTACHECK_TASK_NAME "OTA-Update"
#define OTACHECK_TASK_STACKSIZE (8000)

#define OTACHECK_HTTP_GET_TIMEOUT (15000)

#define UPDATE_PROGRESS(_ofOne, _eResult) \
    do { \
        m_sProgress.dProgressOfOne = _ofOne; \
        m_sProgress.eResult = _eResult; \
    } while(0)

static esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt);
static bool DownloadOTAInfo(const char* url);
static void OTAUpdateTask(void* pParam);

static esp_http_client_config_t GetClientConfig(const char* url);

#define BUFFER_SIZE (4096)
static char m_datas[BUFFER_SIZE] = {0,};

// ---------------------------
// OTA items
static OTACHECK_SOTAItem m_sOTAItems[OTACHECK_SOTAITEM_COUNT];
static int32_t m_s32OTAItemCount = 0;
static int32_t m_s32OTAItemIndex = 0;

// It's not critical so no semaphore lock on this one.
static OTACHECK_SProgress m_sProgress = {0};

static SemaphoreHandle_t m_hOTAListSemaphore;

static TaskHandle_t m_hTaskHandle = NULL;

void OTACHECK_Init()
{
    m_hOTAListSemaphore = xSemaphoreCreateMutex();

    UPDATE_PROGRESS(0.0d, OTACHECK_ERESULT_Idle);
}

bool OTACHECK_InstallOTA(uint32_t u32Id, uint32_t u32TimeoutMS)
{
    bool ret = false;
    OTACHECK_SOTAItem* psSelectedOTAItem = NULL;

    // Don't check until the semaphore is free.
    bool bHasSemaphore = (pdTRUE == xSemaphoreTakeRecursive(m_hOTAListSemaphore, pdMS_TO_TICKS(u32TimeoutMS)));
    if (!bHasSemaphore) {
        goto ERROR;
    }

    if (m_hTaskHandle != NULL)
    {
        ESP_LOGE(TAG, "OTA installation is already in progress");
        goto ERROR;
    }

    // Check if the OTA item exists.
    bool isFound = false;

    for(uint32_t u32Index = 0; u32Index < m_s32OTAItemCount; u32Index++)
    {
        const OTACHECK_SOTAItem* pOTAItem = &m_sOTAItems[u32Index];
        ESP_LOGI(TAG, "id: %" PRIu32 ", version: %s, URL: '%s'", pOTAItem->u32Id, pOTAItem->szVersion, pOTAItem->szURL);
        if (pOTAItem->u32Id == u32Id)
        {
            psSelectedOTAItem = (OTACHECK_SOTAItem*)malloc(sizeof(OTACHECK_SOTAItem)); // I'm doing this despite my EGO
            if (psSelectedOTAItem == NULL)
            {
                ESP_LOGE(TAG, "Not enough memory");
                goto ERROR;
            }
            *psSelectedOTAItem = *pOTAItem;
            isFound = true;
            break;
        }
    }

    // Select OTA
    if (!isFound)
    {
        ESP_LOGE(TAG, "InstallOTA: cannot find the id: %" PRIu32, u32Id);
        goto ERROR;
    }

    if (pdTRUE != xTaskCreate(OTAUpdateTask, OTACHECK_TASK_NAME, OTACHECK_TASK_STACKSIZE, psSelectedOTAItem, OTACHECK_TASK_PRIORITY, &m_hTaskHandle))
    {
        ESP_LOGE(TAG, "Unable to create the OTA upgrade task");
        goto ERROR;
    }

    ret = true;
    goto END;
    ERROR:
    ret = false;
    // If there is no error, the object will be used somewhere else so it won't go in END:
    if (psSelectedOTAItem != NULL) {
        free((OTACHECK_SOTAItem*)psSelectedOTAItem);
    }
    END:
    if (bHasSemaphore) {
        xSemaphoreGive(m_hOTAListSemaphore);
    }
    return ret;
}

static void OTAUpdateTask(void* pParam)
{
    const OTACHECK_SOTAItem* pOTAItem = (const OTACHECK_SOTAItem*)pParam;
    esp_ota_handle_t update_handle = 0;
    esp_http_client_handle_t h = NULL;

    UPDATE_PROGRESS(0, OTACHECK_ERESULT_Progress);

    ESP_LOGI(TAG, "InstallOTA: '%s'", pOTAItem->szURL);

    const esp_http_client_config_t config = GetClientConfig(pOTAItem->szURL);
    h = esp_http_client_init(&config);

    ESP_LOGD(TAG, "HTTP connection in progress ... url: %s", config.url);
    esp_err_t err;
    if (ESP_OK != (err = esp_http_client_open(h, 0)))
    {
        ESP_LOGE(TAG, "Unable to open HTTP client connection, returned: %d, text: %s", err, esp_err_to_name(err));
        goto ERROR;
    }
    ESP_LOGI(TAG, "HTTP connection succeeded! ");

    esp_http_client_fetch_headers(h);
    const int statusCode = esp_http_client_get_status_code(h);
    if (statusCode != 200)
    {
        ESP_LOGE(TAG, "HTTP server didn't return 200, it returned: %d", statusCode);
        goto ERROR;
    }

    const int len = esp_http_client_get_content_length(h);
    //
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

    if (ESP_OK != (err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle)))
    {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        goto ERROR;
    }

    // Put it at 10% just to show some progress
    UPDATE_PROGRESS(0.1d, OTACHECK_ERESULT_Progress);

    int offset = 0;
    TickType_t tt = xTaskGetTickCount();
    while(offset < len)
    {
        int n = esp_http_client_read(h, (char*)m_datas, sizeof(m_datas));
        if (n < 0) { // If we didn't receive correct byte count we call it an error
            ESP_LOGE(TAG, "HTTP Read error, it returned: %d", statusCode);
            goto ERROR;
        }

        if (ESP_OK != (err = esp_ota_write( update_handle, (const void *)m_datas, n)))
        {
            ESP_LOGE(TAG, "OTA write failed, it returned: %d", statusCode);
            goto ERROR;
        }
        offset += n;

        // Don't report progress too often.
        if ( pdTICKS_TO_MS(xTaskGetTickCount() - tt) > 500 ) {
            tt = xTaskGetTickCount();
            const double ofOne = ((double)offset/(double)len);
            ESP_LOGI(TAG, "http read, offset: %d, len: %d, percent: %.2f %%", offset, len, ofOne * 100.0d );
            UPDATE_PROGRESS((0.1d + ofOne*0.9d), OTACHECK_ERESULT_Progress);
        }
    }

    if (ESP_OK != (err = esp_ota_end(update_handle)))
    {
        ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        goto ERROR;
    }

    if (ESP_OK != (esp_ota_set_boot_partition(update_partition)))
    {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        goto ERROR;
    }

    UPDATE_PROGRESS(1.0d, OTACHECK_ERESULT_Progress);
    // Some delay to let the user know that something happened.
    vTaskDelay(pdMS_TO_TICKS(4000));
    UPDATE_PROGRESS(1.0d, OTACHECK_ERESULT_Idle);
    esp_restart();
    goto END;
    ERROR:
    UPDATE_PROGRESS(0.0d, OTACHECK_ERESULT_Error);
    END:
    if (update_handle != 0) {
        esp_ota_abort(update_handle);
    }
    if (h != NULL) {
        esp_http_client_close(h);
        esp_http_client_cleanup(h);
    }
    m_hTaskHandle = NULL;
    vTaskDelete(NULL);
}

bool OTACHECK_CheckOTAvailability(uint32_t u32TimeoutMS)
{
    bool ret = false;
    cJSON* pRoot = NULL;

    // Don't check until the semaphore is free.
    bool bHasSemaphore = (pdTRUE == xSemaphoreTakeRecursive(m_hOTAListSemaphore, pdMS_TO_TICKS(u32TimeoutMS)));
    if (!bHasSemaphore)
        goto ERROR;

    ESP_LOGI(TAG, "Checking OTA availability ...");

    if (!DownloadOTAInfo(FWCONFIG_OTA_URL)) {
        goto ERROR;
    }

    //ESP_LOGI(TAG, "esp_http_client_read, data: '%s'", m_datas);
    pRoot = cJSON_Parse((const char*)m_datas);
    if (pRoot == NULL) {
        goto ERROR;
    }

    const cJSON* pListJSONItems = cJSON_GetObjectItemCaseSensitive(pRoot, "updateBoardNovika");
    if (pListJSONItems == NULL || !cJSON_IsArray(pListJSONItems)) {
        ESP_LOGE(TAG, "Invalid JSON, 'updateBoardNovika' element is missing or not an array");
        goto ERROR;
    }

    m_s32OTAItemCount = 0;

    const cJSON* pListJSON = NULL;
    uint32_t u32Id = (esp_random() % 100000) + 1000;

    cJSON_ArrayForEach(pListJSON, pListJSONItems)
    {
        cJSON* pURLJSON = cJSON_GetObjectItemCaseSensitive(pListJSON, "url");
        if (pURLJSON == NULL || !cJSON_IsString(pURLJSON)) {
            ESP_LOGE(TAG, "Invalid JSON, 'url' is missing");
            goto ERROR;
        }
        cJSON* pVersionJSON  = cJSON_GetObjectItemCaseSensitive(pListJSON, "version");
        if (pVersionJSON == NULL || !cJSON_IsString(pVersionJSON)) {
            ESP_LOGE(TAG, "Invalid JSON, 'version' is missing");
            goto ERROR;
        }
        cJSON* pChangeLogJSON = cJSON_GetObjectItemCaseSensitive(pListJSON, "changeLog");
        if (pChangeLogJSON == NULL || !cJSON_IsString(pChangeLogJSON)) {
            ESP_LOGE(TAG, "Invalid JSON, 'changeLog' is missing");
            goto ERROR;
        }

        // Limited to 4 OTA
        if (m_s32OTAItemCount + 1 > OTACHECK_SOTAITEM_COUNT)
        {
            // Limit reached.
            break;
        }

        OTACHECK_SOTAItem* pOTAItem = &m_sOTAItems[m_s32OTAItemCount];
        pOTAItem->u32Id = u32Id++;
        snprintf(pOTAItem->szURL, sizeof(pOTAItem->szURL), pURLJSON->valuestring);
        snprintf(pOTAItem->szVersion, sizeof(pOTAItem->szVersion), pVersionJSON->valuestring);
        snprintf(pOTAItem->szChangeLogs, sizeof(pOTAItem->szChangeLogs), pChangeLogJSON->valuestring);
        m_s32OTAItemCount++;
    }
    ret = true;
    goto END;
    ERROR:
    ret = false;
    END:
    if (pRoot != NULL) {
        cJSON_free(pRoot);
    }
    if (bHasSemaphore) {
        xSemaphoreGive(m_hOTAListSemaphore);
    }
    return ret;
}

static bool DownloadOTAInfo(const char* url)
{
    bool ret = false;
    esp_http_client_handle_t h = NULL;
    const esp_http_client_config_t config = GetClientConfig(url);
    h = esp_http_client_init(&config);

    ESP_LOGD(TAG, "HTTP connection in progress ... url: %s", config.url);
    esp_err_t err;
    if (ESP_OK != (err = esp_http_client_open(h, 0)))
    {
        ESP_LOGE(TAG, "Unable to open HTTP client connection, returned: %d, text: %s", err, esp_err_to_name(err));
        goto ERROR;
    }
    ESP_LOGI(TAG, "HTTP connection succeeded! ");

    esp_http_client_fetch_headers(h);
    const int statusCode = esp_http_client_get_status_code(h);
    if (statusCode != 200)
    {
        ESP_LOGE(TAG, "HTTP server didn't return 200, it returned: %d", statusCode);
        goto ERROR;
    }

    const int len = esp_http_client_get_content_length(h);
    if (len > BUFFER_SIZE - 1)
    {
        ESP_LOGE(TAG, "JSON file is too big");
        goto ERROR;
    }

    // Read
    int offset = 0;
    while(offset < len)
    {
        int n = esp_http_client_read(h, (char*)m_datas + offset, sizeof(m_datas) - offset - 1);
        if (n < 0) { // If we didn't receive correct byte count we call it an error
            ESP_LOGE(TAG, "HTTP Read error, it returned: %d", statusCode);
            goto ERROR;
        }

        offset += n;
        m_datas[offset] = 0;
    }

    ret = true;
    goto END;
    ERROR:
    ret = false;
    END:
    if (h != NULL) {
        esp_http_client_close(h);
        esp_http_client_cleanup(h);
    }
    return ret;
}

void OTACHECK_OTAItemBegin()
{
    xSemaphoreTakeRecursive(m_hOTAListSemaphore, portMAX_DELAY);
    m_s32OTAItemIndex = 0;
    xSemaphoreGive(m_hOTAListSemaphore);
}

bool OTACHECK_OTAItemGet(OTACHECK_SOTAItem* pOut)
{
    if (pdTRUE != xSemaphoreTakeRecursive(m_hOTAListSemaphore, portMAX_DELAY))
        return false;
    bool ret;
    if (m_s32OTAItemIndex + 1 > m_s32OTAItemCount)
        goto ERROR;
    *pOut = m_sOTAItems[m_s32OTAItemIndex];
    m_s32OTAItemIndex++;
    ret = true;
    goto END;
    ERROR:
    ret = false;
    END:
    xSemaphoreGive(m_hOTAListSemaphore);
    return ret;
}

OTACHECK_SProgress OTACHECK_GetProgress()
{
    return m_sProgress;
}

esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id)
    {
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

static esp_http_client_config_t GetClientConfig(const char* url)
{
    const esp_http_client_config_t config =
    {
        .url = url,
        .auth_type = HTTP_AUTH_TYPE_NONE,
        .method = HTTP_METHOD_GET,
        .timeout_ms = OTACHECK_HTTP_GET_TIMEOUT,

        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        // CRT bundle.
        .crt_bundle_attach = esp_crt_bundle_attach,
        .use_global_ca_store = true,

        .skip_cert_common_name_check = true,

        .disable_auto_redirect = false,

        .event_handler = client_event_get_handler,
    };
    return config;
}
