#ifndef _WEBSERVER_COMMON_H_
#define _WEBSERVER_COMMON_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <esp_http_server.h>
#include "apiurl.h"
#include "esp_log.h"
#include "assets/EmbeddedFiles.h"

#define CHECK_FOR_ACCESS_OR_RETURN() do { \
    if (!g_bHasAccess) { \
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "You don't have access to these resources"); \
        httpd_resp_set_hdr(req, "Connection", "close"); \
        return ESP_FAIL; \
    } \
} while(0)

/* Max length a file path can have on storage */
#define HTTPSERVER_BUFFERSIZE (1024*10)

extern bool g_bHasAccess;

extern uint8_t g_u8Buffers[HTTPSERVER_BUFFERSIZE];

#endif