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
#include "URL.h"
#include "../uartbridge/stovemb.h"
#include "OTAUploadESP32.h"
#include "OTAUploadSTM32.h"
#include "StaticFileServe.h"
#include "APIGet.h"
#include "APIPost.h"

#define TAG "webserver"

uint8_t g_u8Buffers[HTTPSERVER_BUFFERSIZE];

// static bool m_bIsPairing = false;
#if FWCONFIG_DEVMODE != 0
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
    .handler   = APIPOST_action_post_handler,
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
    WSSFS_Init();

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
