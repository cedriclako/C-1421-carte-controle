#include "esp_log.h"
#include "esp_vfs.h"
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "event.h"
#include "Common.h"
#include "webserver.h"
#include "../espnowprocess.h"
#include "../settings.h"
#include "../fwconfig.h"
#include "apiurl.h"
#include "../uartbridge/stovemb.h"
#include "OTAUploadESP32.h"
#include "OTAUploadSTM32.h"
#include "StaticFileServe.h"
#include "APIGet.h"
#include "APIPost.h"

#define TAG "webserver"

static esp_err_t file_post_handler(httpd_req_t *req);

uint8_t g_u8Buffers[HTTPSERVER_BUFFERSIZE];

// static bool m_bIsPairing = false;
#if FWCONFIG_MAINTENANCEACCESS_NOPASSWORD != 0
bool g_bHasAccess = true;
#else
bool g_bHasAccess = false;
#endif

static const httpd_uri_t m_sHttpUI = {
    .uri       = "/*",
    .method    = HTTP_GET,
    .handler   = WSSFS_file_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = ""
};

static const httpd_uri_t m_sHttpGetAPI = {
    .uri       = "/api/*",
    .method    = HTTP_GET,
    .handler   = APIGET_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = ""
};

static const httpd_uri_t m_sHttpPostAPI = {
    .uri       = "/api/*",
    .method    = HTTP_POST,
    .handler   = APIPOST_post_handler,
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

static const httpd_uri_t m_sHttpOTAUploadESP32Post = {
    .uri       = API_POST_OTAUPLOADESP32_URI,
    .method    = HTTP_POST,
    .handler   = OTAUPLOADESP32_postotauploadESP32_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = ""
};

static const httpd_uri_t m_sHttpOTAUploadSTM32Post = {
    .uri       = API_POST_OTAUPLOADSTM32_URI,
    .method    = HTTP_POST,
    .handler   = OTAUPLOADSTM32_postotauploadSTM32_handler,
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
        httpd_register_uri_handler(server, &m_sHttpOTAUploadESP32Post);
        httpd_register_uri_handler(server, &m_sHttpOTAUploadSTM32Post);
    }
}

static esp_err_t file_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "file_post_handler, url: %s", req->uri);
    CHECK_FOR_ACCESS_OR_RETURN();

    if (strcmp(req->uri, ACTION_POST_REBOOT) == 0)
    {
        esp_restart();
    }
    else if (strcmp(req->uri, ACTION_POST_DOWNLOADCONFIG) == 0)
    {
        esp_event_post_to(EVENT_g_LoopHandle, MAINAPP_EVENT, REQUESTCONFIGRELOAD_EVENT, NULL, 0, 0);
    }/*
    else if (strcmp(req->uri, ACTION_POST_ESPNOW_STARTPAIRING) == 0)
    {
        m_bIsPairing = true;
        ESP_LOGI(TAG, "Starting pairing");
    }
    else if (strcmp(req->uri, ACTION_POST_ESPNOW_STOPPAIRING) == 0)
    {
        m_bIsPairing = false;
        ESP_LOGI(TAG, "Stopping pairing");
    }*/
    else
    {
        ESP_LOGE(TAG, "Unknown request for url: %s", req->uri);
        goto ERROR;
    }
 
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
    ERROR:
    ESP_LOGE(TAG, "Invalid request");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad request");
    return ESP_FAIL;
}

/*
    char queryString[64+1] = {0};
    esp_err_t get_query_err;
    if (ESP_OK != (get_query_err = httpd_req_get_url_query_str(req, queryString, sizeof(queryString)-1)))
    {
        ESP_LOGE(TAG, "invalid query string, error: %s", esp_err_to_name(get_query_err));
        goto ERROR;
    }

    ESP_LOGI(TAG, "api_postaccessmaintenanceredirect_handler, url: '%s', query: '%s'", req->uri, queryString);
    char password[16+1] = {0};
    // It seems it already handle the trailing 0. So no need to add -1.
    if (ESP_OK != httpd_query_key_value(queryString, "password", password, sizeof(password)))
    {
        ESP_LOGE(TAG, "invalid query, password key field is not there");
        goto ERROR;
    }
*/