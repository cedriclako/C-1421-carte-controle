#include "esp_log.h"
#include "esp_vfs.h"
#include <stdio.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "esp_app_format.h"
#include "assets/EmbeddedFiles.h"
#include "esp_ota_ops.h"
#include "cJSON.h"

#include "event.h"
#include "webserver.h"
#include "../espnowprocess.h"
#include "nvsjson.h"
#include "../settings.h"
#include "../main.h"
#include "../fwconfig.h"
#include "apiurl.h"
#include "../uartbridge/stovemb.h"

#define TAG "webserver"

/* Max length a file path can have on storage */
#define HTTPSERVER_BUFFERSIZE (1024*10)

static esp_err_t api_get_handler(httpd_req_t *req);
static esp_err_t api_post_handler(httpd_req_t *req);

static esp_err_t file_get_handler(httpd_req_t *req);
static esp_err_t file_post_handler(httpd_req_t *req);
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename);

static esp_err_t file_otauploadpost_handler(httpd_req_t *req);

static const EF_SFile* GetFile(const char* strFilename);

static char* GetSysInfo();
static char* GetLiveData();
static void ToHexString(char *dstHexString, const uint8_t* data, uint8_t len);
static const char* GetESPChipId(esp_chip_model_t eChipid);

static uint8_t m_u8Buffers[HTTPSERVER_BUFFERSIZE];

/*! @brief this variable is set by linker script, don't rename it. It contains app image informations. */
extern const esp_app_desc_t esp_app_desc;

static bool m_bIsPairing = false;

static const httpd_uri_t m_sHttpUI = {
    .uri       = "/*",
    .method    = HTTP_GET,
    .handler   = file_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = ""
};

static const httpd_uri_t m_sHttpGetAPI = {
    .uri       = "/api/*",
    .method    = HTTP_GET,
    .handler   = api_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = ""
};

static const httpd_uri_t m_sHttpPostAPI = {
    .uri       = "/api/*",
    .method    = HTTP_POST,
    .handler   = api_post_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = ""
};
static const httpd_uri_t m_sHttpActionPost = {
    .uri       = "/action/*",
    .method    = HTTP_POST,
    .handler   = file_post_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = ""
};

static const httpd_uri_t m_sHttpOTAUploadPost = {
    .uri       = "/ota/upload",
    .method    = HTTP_POST,
    .handler   = file_otauploadpost_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = ""
};

void WEBSERVER_Init()
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.task_priority = FWCONFIG_HTTPTASK_PRIORITY;
    config.stack_size = 9500;
    config.lru_purge_enable = true;
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_open_sockets = 13;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &m_sHttpActionPost);
        httpd_register_uri_handler(server, &m_sHttpGetAPI);
        httpd_register_uri_handler(server, &m_sHttpPostAPI);
        httpd_register_uri_handler(server, &m_sHttpUI);
        // return server;
        httpd_register_uri_handler(server, &m_sHttpOTAUploadPost);
    }
}

/* An HTTP GET handler */
static esp_err_t file_get_handler(httpd_req_t *req)
{
    const EF_SFile* pFile = NULL;

    ESP_LOGI(TAG, "Opening file uri: %s", req->uri);

    // Redirect root to index.html
    if (strcmp(req->uri, "/") == 0)
    {
        pFile = GetFile(DEFAULT_RELATIVE_URI+1);
    }
    else {
        pFile = GetFile(req->uri+1);
    }

    if (pFile == NULL)
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File does not exist");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, pFile->strFilename);

    uint32_t u32Index = 0;

    while(u32Index < pFile->u32Length)
    {
        const uint32_t n = MIN(pFile->u32Length - u32Index, HTTPSERVER_BUFFERSIZE);

        if (n > 0) {
            /* Send the buffer contents as HTTP response m_u8Buffers */
            if (httpd_resp_send_chunk(req, (char*)(pFile->pu8StartAddr + u32Index), n) != ESP_OK) {
                ESP_LOGE(TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
           }
        }
        u32Index += n;
    }

    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t file_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "file_post_handler, url: %s", req->uri);

    if (strcmp(req->uri, ACTION_POST_REBOOT) == 0)
    {
        esp_restart();
    }
    else if (strcmp(req->uri, ACTION_POST_DOWNLOADCONFIG) == 0)
    {
        esp_event_post_to(EVENT_g_LoopHandle, MAINAPP_EVENT, REQUESTCONFIGRELOAD_EVENT, NULL, 0, 0);
    }
    else if (strcmp(req->uri, ACTION_POST_ESPNOW_STARTPAIRING) == 0)
    {
        m_bIsPairing = true;
        ESP_LOGI(TAG, "Starting pairing");
    }
    else if (strcmp(req->uri, ACTION_POST_ESPNOW_STOPPAIRING) == 0)
    {
        m_bIsPairing = false;
        ESP_LOGI(TAG, "Stopping pairing");
    }
    else
    {
        ESP_LOGE(TAG, "Unknown request for url: %s", req->uri);
        goto ERROR;
    }
    return ESP_OK;
    ERROR:
    ESP_LOGE(TAG, "Invalid request");
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad request");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_FAIL;
}

static esp_err_t api_get_handler(httpd_req_t *req)
{
    //ESP_LOGI(TAG, "api_get_handler, url: %s", req->uri);
    char* szErrorString = NULL;
    char* pExportJSON = NULL;

    bool bIsChunk = false;

    if (strcmp(req->uri, API_GETSETTINGSJSON_URI) == 0)
    {
        pExportJSON = NVSJSON_ExportJSON(&g_sSettingHandle);

        if (pExportJSON == NULL || httpd_resp_send_chunk(req, pExportJSON, strlen(pExportJSON)) != ESP_OK)
            goto ERROR;
        bIsChunk = true;
    }
    else if (strcmp(req->uri, API_GETSYSINFOJSON_URI) == 0)
    {
        pExportJSON = GetSysInfo();
        if (pExportJSON == NULL || httpd_resp_send_chunk(req, pExportJSON, strlen(pExportJSON)) != ESP_OK)
            goto ERROR;
        bIsChunk = true;
    }
    else if (strcmp(req->uri, API_GETLIVEDATAJSON_URI) == 0)
    {
        pExportJSON = GetLiveData();
        if (pExportJSON == NULL || httpd_resp_send_chunk(req, pExportJSON, strlen(pExportJSON)) != ESP_OK)
            goto ERROR;
        bIsChunk = true;
    }
    else if (strcmp(req->uri, API_GETSERVERPARAMETERFILEJSON_URI) == 0)
    {
        pExportJSON = STOVEMB_ExportParamToJSON();
        if (pExportJSON == NULL || httpd_resp_send_chunk(req, pExportJSON, strlen(pExportJSON)) != ESP_OK)
        {
            szErrorString = "Server parameter file is not available";
            goto ERROR;
        }
        bIsChunk = true;
    }
    else
    {
        ESP_LOGE(TAG, "api_get_handler, url: %s", req->uri);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Unknown request");
    }
    
    goto END;
    ERROR:
    if (szErrorString != NULL)
    {
        ESP_LOGE(TAG, "api_get_handler, url: %s, error: %s", req->uri, szErrorString);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, szErrorString);
    }
    else
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "request processing error");
    END:
    if (pExportJSON != NULL)
        free(pExportJSON);

    httpd_resp_set_hdr(req, "Connection", "close");
    // REALLY IMPORTANT, only send this in chunk mode or the browser will hang
    if (bIsChunk)
        httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t api_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "test");
    char szError[128+1] = {0,};

    const int total_len = req->content_len;

    if (total_len >= HTTPSERVER_BUFFERSIZE-1)
    {
        /* Respond with 500 Internal Server Error */
        ESP_LOGE(TAG, "content too long");
        goto ERROR;
    }

    // Receive the complete payload.
    int n = 0;
    while (n < total_len)
    {
        const int received = httpd_req_recv(req, (char*)(m_u8Buffers + n), total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            ESP_LOGE(TAG, "Failed to post control value");
            goto ERROR;
        }
        n += received;
    }
    m_u8Buffers[n] = '\0';

    ESP_LOGI(TAG, "api_post_handler, url: %s", req->uri);
    if (strcmp(req->uri, API_POSTSETTINGSJSON_URI) == 0)
    {
        if (!NVSJSON_ImportJSON(&g_sSettingHandle, (const char*)m_u8Buffers))
        {
            snprintf(szError, sizeof(szError), "%s", "Unable to import JSON");
            goto ERROR;
        }
    }
    else if (strcmp(req->uri, API_POSTSERVERPARAMETERFILEJSON_URI) == 0)
    {
        ESP_LOGI(TAG, "api_post_handler, url: %s, json len: %d", req->uri, n);

        if (!STOVEMB_InputParamFromJSON((const char*)m_u8Buffers, szError, sizeof(szError)))
        {
            goto ERROR;
        }
        esp_event_post_to(EVENT_g_LoopHandle, MAINAPP_EVENT, REQUESTCONFIGWRITE_EVENT, NULL, 0, 0);
    }
    else
    {
        ESP_LOGE(TAG, "api_post_handler, url: %s", req->uri);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Unknown request");
    }
    goto END;
    ERROR:
    if (strlen(szError) > 0)
    {
        ESP_LOGE(TAG, "api_post_handler, url: %s, error: %s", req->uri, szError);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, szError);
    }
    else
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "unknown error");
    END:
    httpd_resp_set_hdr(req, "Connection", "close");
    //httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

#define IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)
    
/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename)
{
    if (IS_FILE_EXT(filename, ".pdf")) {
        return httpd_resp_set_type(req, "application/pdf");
    } else if (IS_FILE_EXT(filename, ".html") | IS_FILE_EXT(filename, ".htm")) {
        return httpd_resp_set_type(req, "text/html");
    } else if (IS_FILE_EXT(filename, ".jpeg") || IS_FILE_EXT(filename, ".jpg")) {
        return httpd_resp_set_type(req, "image/jpeg");
    } else if (IS_FILE_EXT(filename, ".ico")) {
        return httpd_resp_set_type(req, "image/x-icon");
    } else if (IS_FILE_EXT(filename, ".css")) {
        return httpd_resp_set_type(req, "text/css");
    } else if (IS_FILE_EXT(filename, ".txt")) {
        return httpd_resp_set_type(req, "text/plain");
    } else if (IS_FILE_EXT(filename, ".js")) {
        return httpd_resp_set_type(req, "text/javascript");
    } else if (IS_FILE_EXT(filename, ".json")) {
        return httpd_resp_set_type(req, "application/json");
    }
    else if (IS_FILE_EXT(filename, ".ttf")) {
        return httpd_resp_set_type(req, "application/x-font-truetype");
    }
    else if (IS_FILE_EXT(filename, ".woff")) {
        return httpd_resp_set_type(req, "application/font-woff");
    }
    else if (IS_FILE_EXT(filename, ".svg")) {
        return httpd_resp_set_type(req, "image/svg+xml");
    }
    
    /* This is a limited set only */
    /* For any other type always set as plain text */
    return httpd_resp_set_type(req, "text/plain");
}

static const EF_SFile* GetFile(const char* strFilename)
{
    for(int i = 0; i < EF_EFILE_COUNT; i++)
    {
        const EF_SFile* pFile = &EF_g_sFiles[i];
        if (strcmp(pFile->strFilename, strFilename) == 0)
            return pFile;
    }

    return NULL;
}

static char* GetSysInfo()
{
    cJSON* pRoot = NULL;

    char buff[100];
    pRoot = cJSON_CreateObject();
    if (pRoot == NULL)
        goto ERROR;
 
    cJSON* pEntries = cJSON_AddArrayToObject(pRoot, "infos");

    esp_chip_info_t sChipInfo;
    esp_chip_info(&sChipInfo);

    // Chip
    cJSON* pEntryJSON0 = cJSON_CreateObject();
    cJSON_AddItemToObject(pEntryJSON0, "name", cJSON_CreateString("Chip"));
    cJSON_AddItemToObject(pEntryJSON0, "value", cJSON_CreateString(GetESPChipId(sChipInfo.model)));
    cJSON_AddItemToArray(pEntries, pEntryJSON0);

    // Firmware
    cJSON* pEntryJSON1 = cJSON_CreateObject();
    cJSON_AddItemToObject(pEntryJSON1, "name", cJSON_CreateString("Firmware"));
    cJSON_AddItemToObject(pEntryJSON1, "value", cJSON_CreateString(esp_app_desc.version));
    cJSON_AddItemToArray(pEntries, pEntryJSON1);

    // Compile Time
    cJSON* pEntryJSON2 = cJSON_CreateObject();
    cJSON_AddItemToObject(pEntryJSON2, "name", cJSON_CreateString("Compile Time"));
    sprintf(buff, "%s %s", /*0*/esp_app_desc.date, /*0*/esp_app_desc.time);
    cJSON_AddItemToObject(pEntryJSON2, "value", cJSON_CreateString(buff));
    cJSON_AddItemToArray(pEntries, pEntryJSON2);

    // SHA256
    cJSON* pEntryJSON3 = cJSON_CreateObject();
    cJSON_AddItemToObject(pEntryJSON3, "name", cJSON_CreateString("SHA256"));
    char elfSHA256[sizeof(esp_app_desc.app_elf_sha256)*2 + 1] = {0,};
    ToHexString(elfSHA256, esp_app_desc.app_elf_sha256, sizeof(esp_app_desc.app_elf_sha256));
    cJSON_AddItemToObject(pEntryJSON3, "value", cJSON_CreateString(elfSHA256));
    cJSON_AddItemToArray(pEntries, pEntryJSON3);

    // IDF
    cJSON* pEntryJSON4 = cJSON_CreateObject();
    cJSON_AddItemToObject(pEntryJSON4, "name", cJSON_CreateString("IDF"));
    cJSON_AddItemToObject(pEntryJSON4, "value", cJSON_CreateString(esp_app_desc.idf_ver));
    cJSON_AddItemToArray(pEntries, pEntryJSON4);

    // WiFi-STA
    uint8_t u8Macs[6];
    cJSON* pEntryJSON6 = cJSON_CreateObject();
    cJSON_AddItemToObject(pEntryJSON6, "name", cJSON_CreateString("WiFi.STA"));
    esp_read_mac(u8Macs, ESP_MAC_WIFI_STA);
    sprintf(buff, "%02X:%02X:%02X:%02X:%02X:%02X", /*0*/u8Macs[0], /*1*/u8Macs[1], /*2*/u8Macs[2], /*3*/u8Macs[3], /*4*/u8Macs[4], /*5*/u8Macs[5]);
    cJSON_AddItemToObject(pEntryJSON6, "value", cJSON_CreateString(buff));
    cJSON_AddItemToArray(pEntries, pEntryJSON6);

    // WiFi-AP
    cJSON* pEntryJSON5 = cJSON_CreateObject();
    cJSON_AddItemToObject(pEntryJSON5, "name", cJSON_CreateString("WiFi.AP"));
    esp_read_mac(u8Macs, ESP_MAC_WIFI_SOFTAP);
    sprintf(buff, "%02X:%02X:%02X:%02X:%02X:%02X", /*0*/u8Macs[0], /*1*/u8Macs[1], /*2*/u8Macs[2], /*3*/u8Macs[3], /*4*/u8Macs[4], /*5*/u8Macs[5]);
    cJSON_AddItemToObject(pEntryJSON5, "value", cJSON_CreateString(buff));
    cJSON_AddItemToArray(pEntries, pEntryJSON5);

    // WiFi-BT
    cJSON* pEntryJSON7 = cJSON_CreateObject();
    cJSON_AddItemToObject(pEntryJSON7, "name", cJSON_CreateString("WiFi.BT"));
    esp_read_mac(u8Macs, ESP_MAC_BT);
    sprintf(buff, "%02X:%02X:%02X:%02X:%02X:%02X", /*0*/u8Macs[0], /*1*/u8Macs[1], /*2*/u8Macs[2], /*3*/u8Macs[3], /*4*/u8Macs[4], /*5*/u8Macs[5]);
    cJSON_AddItemToObject(pEntryJSON7, "value", cJSON_CreateString(buff));
    cJSON_AddItemToArray(pEntries, pEntryJSON7);

    // Memory
    cJSON* pEntryJSON8 = cJSON_CreateObject();
    cJSON_AddItemToObject(pEntryJSON8, "name", cJSON_CreateString("Memory"));
    const int totalSize = heap_caps_get_total_size(MALLOC_CAP_8BIT);
    const int usedSize = totalSize - heap_caps_get_free_size(MALLOC_CAP_8BIT);
    
    sprintf(buff, "%d / %d", /*0*/usedSize, /*1*/totalSize);
    cJSON_AddItemToObject(pEntryJSON8, "value", cJSON_CreateString(buff));
    cJSON_AddItemToArray(pEntries, pEntryJSON8);

    // WiFi-station (IP address)
    cJSON* pEntryJSON9 = cJSON_CreateObject();
    cJSON_AddItemToObject(pEntryJSON9, "name", cJSON_CreateString("WiFi (STA)"));
    esp_netif_ip_info_t wifiIpSta;
    MAIN_GetWiFiSTAIP(&wifiIpSta);
    sprintf(buff, IPSTR, IP2STR(&wifiIpSta.ip));
    cJSON_AddItemToObject(pEntryJSON9, "value", cJSON_CreateString(buff));
    cJSON_AddItemToArray(pEntries, pEntryJSON9);

    // WiFi-Soft AP (IP address)
    cJSON* pEntryJSON10 = cJSON_CreateObject();
    cJSON_AddItemToObject(pEntryJSON10, "name", cJSON_CreateString("WiFi (Soft-AP)"));
    esp_netif_ip_info_t wifiIpSoftAP;
    MAIN_GetWiFiSoftAPIP(&wifiIpSoftAP);
    sprintf(buff, IPSTR, IP2STR(&wifiIpSoftAP.ip));
    cJSON_AddItemToObject(pEntryJSON10, "value", cJSON_CreateString(buff));
    cJSON_AddItemToArray(pEntries, pEntryJSON10);

    char* pStr =  cJSON_PrintUnformatted(pRoot);
    cJSON_Delete(pRoot);
    return pStr;
    ERROR:
    cJSON_Delete(pRoot);
    return NULL;
}

static char* GetLiveData()
{
    cJSON* pRoot = NULL;

    pRoot = cJSON_CreateObject();
    if (pRoot == NULL)
        goto ERROR;
        
    cJSON* pState = cJSON_CreateObject();
    cJSON_AddItemToObject(pState, "is_pairing", cJSON_CreateBool(m_bIsPairing));
    cJSON_AddItemToObject(pRoot, "state", pState);

    cJSON* pWireless = cJSON_CreateObject();
    ESPNOWPROCESS_ESPNowInfo sESPNowInfo = ESPNOWPROCESS_GetESPNowInfo();
    cJSON_AddItemToObject(pWireless, "rx", cJSON_CreateNumber(sESPNowInfo.u32RX));
    cJSON_AddItemToObject(pWireless, "tx", cJSON_CreateNumber(sESPNowInfo.u32TX)); 

    wifi_second_chan_t secondChan;
    uint8_t u8Primary;
    esp_wifi_get_channel(&u8Primary,  &secondChan);
    cJSON_AddItemToObject(pWireless, "channel", cJSON_CreateNumber(u8Primary)); 

    cJSON_AddItemToObject(pRoot, "wireless", pWireless);

    STOVEMB_Take(portMAX_DELAY);
    cJSON* pStove = cJSON_CreateObject();
    
    const STOVEMB_SMemBlock* pMemBlock = STOVEMB_GetMemBlockRO();
    cJSON_AddItemToObject(pStove, "is_connected", cJSON_CreateBool(pMemBlock->bIsStoveConnectedAndReady));
    cJSON_AddItemToObject(pStove, "param_cnt", cJSON_CreateNumber(pMemBlock->u32ParameterCount));
    cJSON_AddItemToObject(pStove, "is_param_upload_error", cJSON_CreateBool(pMemBlock->bIsAnyUploadError));
    cJSON_AddItemToObject(pRoot, "stove", pStove);
    STOVEMB_Give();

    char* pStr =  cJSON_PrintUnformatted(pRoot);
    cJSON_Delete(pRoot);
    return pStr;
    ERROR:
    cJSON_Delete(pRoot);
    return NULL;
}

static void ToHexString(char *dstHexString, const uint8_t* data, uint8_t len)
{
    for (uint32_t i = 0; i < len; i++)
        sprintf(dstHexString + (i * 2), "%02X", data[i]);
}

static const char* GetESPChipId(esp_chip_model_t eChipid)
{
    switch(eChipid)
    {
        case CHIP_ESP32:
            return "ESP32";
        case CHIP_ESP32S2:
            return "ESP32-S2";
        case CHIP_ESP32C3:
            return "ESP32-C3";
        case CHIP_ESP32S3:
            return "ESP32-S3";
        case CHIP_ESP32H2:
            return "ESP32-H2";
    }
    return "";
}

static esp_err_t file_otauploadpost_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "file_otauploadpost_handler / uri: %s", req->uri);

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running)
    {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }

    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);

    const esp_partition_t* update_partition = esp_ota_get_next_update_partition(NULL);
    assert(update_partition != NULL);
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);

    esp_ota_handle_t update_handle = 0;

    esp_err_t err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
    err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        esp_ota_abort(update_handle);
        goto ERROR;
    }

    int n = httpd_req_recv(req, (char*)m_u8Buffers, HTTPSERVER_BUFFERSIZE);
    int binary_file_length = 0;

    while(n > 0)
    {
        ESP_LOGI(TAG, "file_otauploadpost_handler / receiving: %d bytes", n);

        err = esp_ota_write( update_handle, (const void *)m_u8Buffers, n);
        if (err != ESP_OK)
        {
            esp_ota_abort(update_handle);
            goto ERROR;
        }
        binary_file_length += n;
        ESP_LOGD(TAG, "Written image length %d", binary_file_length);

        n = httpd_req_recv(req, (char*)m_u8Buffers, HTTPSERVER_BUFFERSIZE);
    }

    err = esp_ota_end(update_handle);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        } else {
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        }

        // TODO: esp_ota_abort(update_handle); needed ?
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
    return ESP_OK;
    ERROR:
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid image");
    return ESP_FAIL;
}