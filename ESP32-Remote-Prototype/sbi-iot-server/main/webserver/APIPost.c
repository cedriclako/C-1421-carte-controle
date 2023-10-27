#include "APIPost.h"
#include "../settings.h"
#include "../espnowprocess.h"
#include "../uartbridge/stovemb.h"
#include "../main.h"
#include "../Event.h"
#include "esp_log.h"
#include "cJson.h"
#include "fwconfig.h"

#define TAG "APIPOST"

static bool PostPairingSetting(const char* szJSON);
static bool PostWifiSetting(const char* szJSON);

static bool HandleMaintenancePasswordRequest(const char* szJSON);

esp_err_t APIPOST_post_handler(httpd_req_t *req)
{
    esp_err_t esperr = ESP_OK;
    
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
        const int received = httpd_req_recv(req, (char*)(g_u8Buffers + n), total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            ESP_LOGE(TAG, "Failed to post control value");
            goto ERROR;
        }
        n += received;
    }
    g_u8Buffers[n] = '\0';

    ESP_LOGI(TAG, "api_post_handler, url: %s", req->uri);
    if (strcmp(req->uri, API_POSTSETTINGSJSON_URI) == 0)
    {
        CHECK_FOR_ACCESS_OR_RETURN(); // STOP THERE is the user doesn't have access
        if (!NVSJSON_ImportJSON(&g_sSettingHandle, (const char*)g_u8Buffers))
        {
            snprintf(szError, sizeof(szError), "%s", "Unable to import JSON");
            goto ERROR;
        }
    }
    else if (strcmp(req->uri, API_GETPOST_SERVERPARAMETERFILEJSON_URI) == 0)
    {
        CHECK_FOR_ACCESS_OR_RETURN(); // STOP THERE is the user doesn't have access
        ESP_LOGI(TAG, "api_post_handler, url: %s, json len: %d", req->uri, n);
        if (!STOVEMB_InputParamFromJSON((const char*)g_u8Buffers, szError, sizeof(szError)))
        {
            goto ERROR;
        }
        esp_event_post_to(EVENT_g_LoopHandle, MAINAPP_EVENT, REQUESTCONFIGWRITE_EVENT, NULL, 0, 0);
    }
    else if (strcmp(req->uri, API_POST_ACCESSMAINTENANCEREDIRECT_URI) == 0)
    {
        if (!HandleMaintenancePasswordRequest((const char*)g_u8Buffers))
        {
            g_bHasAccess = false;
            snprintf(szError, sizeof(szError), "%s", "Invalid password");
            goto ERROR;
        }
        g_bHasAccess = true;
        CHECK_FOR_ACCESS_OR_RETURN();
        httpd_resp_set_status(req, "302 Moved Permanently");
        httpd_resp_set_hdr(req, "Location", "/mnt-index.html");
    }
    else if (strcmp(req->uri, API_GETPOST_PAIRINGSETTINGS) == 0)
    {
        if (!PostPairingSetting((const char*)g_u8Buffers))
        {
            snprintf(szError, sizeof(szError), "%s", "Invalid mac address");
            goto ERROR;
        }
    }
    else if (strcmp(req->uri, API_GETPOST_WIFISETTINGS_URI) == 0)
    {
        if (!PostWifiSetting((const char*)g_u8Buffers))
        {
            snprintf(szError, sizeof(szError), "%s", "Invalid Wi-Fi settings");
            goto ERROR;
        }
    }
    goto END;
    ERROR:
    esperr = ESP_FAIL;
    if (strlen(szError) > 0)
    {
        ESP_LOGE(TAG, "api_post_handler, url: %s, error: %s", req->uri, szError);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, szError);
    }
    else
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "unknown error");
    END:
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send_chunk(req, NULL, 0);
    return esperr;
}

static bool PostPairingSetting(const char* szJSON)
{
    bool bRet = true;
    cJSON* pRoot = cJSON_Parse(szJSON);
    if (pRoot == NULL)
        goto ERROR;
    const cJSON* pEntryJSON = cJSON_GetObjectItemCaseSensitive(pRoot, "mac_addr");
    if (pEntryJSON == NULL || !cJSON_IsString(pEntryJSON))
    {
        ESP_LOGE(TAG, "Cannot find JSON mac_addr element");
        goto ERROR;
    }
    NVSJSON_ESETRET eSetRet;
    if ((eSetRet = NVSJSON_SetValueString(&g_sSettingHandle, SETTINGS_EENTRY_ESPNowRemoteMac, false, pEntryJSON->valuestring)) != NVSJSON_ESETRET_OK)
    {
        ESP_LOGE(TAG, "Unable to set value, ret: %"PRId32, (int32_t)eSetRet);
        goto ERROR;
    }
    bRet = true;
    goto END;
    ERROR:
    bRet = false;
    END:
    if (pRoot != NULL)
        cJSON_free(pRoot);
    return bRet;
}

static bool PostWifiSetting(const char* szJSON)
{
    bool bRet = true;
    cJSON* pRoot = cJSON_Parse(szJSON);
    if (pRoot == NULL)
        goto ERROR;

    ESP_LOGI(TAG, "test: %s", szJSON);

    const cJSON* pEnableJSON = cJSON_GetObjectItemCaseSensitive(pRoot, "en");
    const cJSON* pSSIDJSON = cJSON_GetObjectItemCaseSensitive(pRoot, "ssid");
    const cJSON* pPassJSON = cJSON_GetObjectItemCaseSensitive(pRoot, "pass");

    // Invalid JSON file
    if (pEnableJSON == NULL || !cJSON_IsBool(pEnableJSON) ||
        pSSIDJSON == NULL || !cJSON_IsString(pSSIDJSON) ||
        pPassJSON == NULL || !cJSON_IsString(pPassJSON) )
    {
        ESP_LOGE(TAG, "Invalid JSON file");
        goto ERROR;
    }

    // Check if all are possibles.
    if (NVSJSON_ESETRET_OK != NVSJSON_SetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_WSTAIsActive, true, pEnableJSON->valueint))
    {
        ESP_LOGE(TAG, "Invalid Wi-Fi enable state");
        goto ERROR;
    }
    if (NVSJSON_ESETRET_OK != NVSJSON_SetValueString(&g_sSettingHandle, SETTINGS_EENTRY_WSTASSID, true, pSSIDJSON->valuestring))
    {
        ESP_LOGE(TAG, "Invalid Wi-Fi SSID");
        goto ERROR;
    }
    NVSJSON_ESETRET ret;
    if (NVSJSON_ESETRET_OK != (ret = NVSJSON_SetValueString(&g_sSettingHandle, SETTINGS_EENTRY_WSTAPass, true, pPassJSON->valuestring)))
    {
        ESP_LOGE(TAG, "Invalid Wi-Fi password, result: %d", (int)ret);
        goto ERROR;
    }

    NVSJSON_SetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_WSTAIsActive, false, pEnableJSON->valueint);
    NVSJSON_SetValueString(&g_sSettingHandle, SETTINGS_EENTRY_WSTASSID, false, pSSIDJSON->valuestring);
    NVSJSON_SetValueString(&g_sSettingHandle, SETTINGS_EENTRY_WSTAPass, false, pPassJSON->valuestring);
    bRet = true;
    goto END;
    ERROR:
    bRet = false;
    END:
    if (pRoot != NULL)
        cJSON_free(pRoot);
    return bRet;
}

static bool HandleMaintenancePasswordRequest(const char* szJSON)
{
    bool bRet = true;
    cJSON* pRoot = cJSON_Parse(szJSON);
    if (pRoot == NULL)
        goto ERROR;

    const cJSON* pEntryJSON = cJSON_GetObjectItemCaseSensitive(pRoot, "password");
    if (pEntryJSON == NULL || !cJSON_IsString(pEntryJSON))
    {
        ESP_LOGE(TAG, "Cannot find JSON password element");
        goto ERROR;
    }
    #if FWCONFIG_MAINTENANCEACCESS_NOPASSWORD == 0
    if (strcmp((const char*)pEntryJSON->valuestring, FWCONFIG_MAINTENANCEACCESS_PASSWORD) != 0)
    {
        ESP_LOGE(TAG, "Wrong password");
        goto ERROR;
    }
    #endif
    bRet = true;
    goto END; 
    ERROR:
    bRet = false;
    END:
    if (pRoot != NULL)
        cJSON_free(pRoot);
    return bRet;
}