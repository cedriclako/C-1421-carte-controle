#include "APIGet.h"
#include "../uartbridge/stovemb.h"
#include "nvsjson.h"
#include "../settings.h"
#include "../espnowprocess.h"
#include "cJSON.h"
#include "../main.h"
#include "esp_mac.h"
#include "esp_chip_info.h"
#include "esp_app_format.h"
#include "esp_ota_ops.h"

#define TAG "APIGet"

static char* GetSysInfo();
static char* GetLiveData();
static char* GetPairingSettingToJSON();
static void ToHexString(char *dstHexString, const uint8_t* data, uint8_t len);
static const char* GetESPChipId(esp_chip_model_t eChipid);

/*! @brief this variable is set by linker script, don't rename it. It contains app image informations. */
extern const esp_app_desc_t esp_app_desc;

esp_err_t APIGET_get_handler(httpd_req_t *req)
{
    esp_err_t esperr = ESP_OK;

    char szError[128+1] = {0,};
    char* pExportJSON = NULL;

    if (strcmp(req->uri, API_GETSETTINGSJSON_URI) == 0)
    {
        CHECK_FOR_ACCESS_OR_RETURN();
        pExportJSON = NVSJSON_ExportJSON(&g_sSettingHandle);

        if (pExportJSON == NULL || httpd_resp_send_chunk(req, pExportJSON, strlen(pExportJSON)) != ESP_OK)
            goto ERROR;
        httpd_resp_set_type(req, "application/json");
    }
    else if (strcmp(req->uri, API_GETSYSINFOJSON_URI) == 0)
    {
        CHECK_FOR_ACCESS_OR_RETURN();
        pExportJSON = GetSysInfo();
        if (pExportJSON == NULL || httpd_resp_send_chunk(req, pExportJSON, strlen(pExportJSON)) != ESP_OK)
            goto ERROR;
        httpd_resp_set_type(req, "application/json");
    }
    else if (strcmp(req->uri, API_GETLIVEDATAJSON_URI) == 0)
    {
        CHECK_FOR_ACCESS_OR_RETURN();
        pExportJSON = GetLiveData();
        if (pExportJSON == NULL || httpd_resp_send_chunk(req, pExportJSON, strlen(pExportJSON)) != ESP_OK)
            goto ERROR;
        httpd_resp_set_type(req, "application/json");
    }
    else if (strcmp(req->uri, API_GETSERVERPARAMETERFILEJSON_URI) == 0)
    {
        CHECK_FOR_ACCESS_OR_RETURN();
        pExportJSON = STOVEMB_ExportParamToJSON();
        if (pExportJSON == NULL || httpd_resp_send_chunk(req, pExportJSON, strlen(pExportJSON)) != ESP_OK)
        {
            strcpy(szError, "Server parameter file is not available");
            goto ERROR;
        }
        httpd_resp_set_type(req, "application/json");
    }
    else if (strcmp(req->uri, API_GETPAIRINGSETTING_URI) == 0)
    {
        pExportJSON = GetPairingSettingToJSON();
        if (pExportJSON == NULL || httpd_resp_send_chunk(req, pExportJSON, strlen(pExportJSON)) != ESP_OK)
            goto ERROR;
        httpd_resp_set_type(req, "application/json");
    }
    else
    {
        ESP_LOGE(TAG, "api_get_handler, url: %s", req->uri);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Unknown request");
        goto END;
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
    if (pExportJSON != NULL)
        free(pExportJSON);
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send_chunk(req, NULL, 0);
    return esperr;
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

    esp_ip6_addr_t if_ip6[CONFIG_LWIP_IPV6_NUM_ADDRESSES] = {0};
    const int32_t s32IPv6Count = MAIN_GetWiFiSTAIPv6(if_ip6);
    for(int i = 0; i < s32IPv6Count; i++)
    {
        char ipv6String[45+1] = {0,};
        snprintf(ipv6String, sizeof(ipv6String)-1, IPV6STR, IPV62STR(if_ip6[i]));

        cJSON* pEntryJSONIPv6 = cJSON_CreateObject();
        cJSON_AddItemToObject(pEntryJSONIPv6, "name", cJSON_CreateString("WiFi (STA) IPv6"));
        cJSON_AddItemToObject(pEntryJSONIPv6, "value", cJSON_CreateString(ipv6String));
        cJSON_AddItemToArray(pEntries, pEntryJSONIPv6);
    }

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
    // Stove
    cJSON* pStove = cJSON_CreateObject();
    const STOVEMB_SMemBlock* pMemBlock = STOVEMB_GetMemBlockRO();
    cJSON_AddItemToObject(pStove, "is_connected", cJSON_CreateBool(pMemBlock->bIsStoveConnectedAndReady));
    cJSON_AddItemToObject(pStove, "param_cnt", cJSON_CreateNumber(pMemBlock->u32ParameterCount));
    cJSON_AddItemToObject(pStove, "is_param_upload_error", cJSON_CreateBool(pMemBlock->bIsAnyUploadError));
    cJSON_AddItemToObject(pStove, "is_param_download_error", cJSON_CreateBool(pMemBlock->bIsAnyDownloadError));
    cJSON_AddItemToObject(pStove, "debug_string", cJSON_CreateString(pMemBlock->szDebugJSONString));
    cJSON_AddItemToObject(pRoot, "stove", pStove);
    // Remote
    cJSON* pRemote = cJSON_CreateObject();
    cJSON_AddItemToObject(pRemote, "rmt_TstatReqBool", cJSON_CreateBool(pMemBlock->sRemoteData.sRMT_TstatReqBool.s32Value));
    cJSON_AddItemToObject(pRemote, "rmt_BoostBool", cJSON_CreateBool(pMemBlock->sRemoteData.sRMT_BoostBool.s32Value));
    cJSON_AddItemToObject(pRemote, "rmt_LowerFanSpeed", cJSON_CreateNumber(pMemBlock->sRemoteData.sRMT_LowerFanSpeed.s32Value));
    cJSON_AddItemToObject(pRemote, "rmt_DistribFanSpeed", cJSON_CreateNumber(pMemBlock->sRemoteData.sRMT_DistribFanSpeed.s32Value));
    
    cJSON_AddItemToObject(pRemote, "tempC_current", cJSON_CreateNumber(pMemBlock->sRemoteData.fTempCurrentC));
    cJSON_AddItemToObject(pRemote, "tempC_sp", cJSON_CreateNumber(pMemBlock->sRemoteData.sTempSetpoint.temp));
    cJSON_AddItemToObject(pRemote, "fanspeed", cJSON_CreateNumber((int)pMemBlock->sRemoteData.eFanSpeedCurr));

    const TickType_t ttLastCommTicks = xTaskGetTickCount() - pMemBlock->sRemoteData.ttLastCommunicationTicks;
    cJSON_AddItemToObject(pRemote, "lastcomm_ms", cJSON_CreateNumber(pdTICKS_TO_MS(ttLastCommTicks)));

    cJSON_AddItemToObject(pRoot, "remote", pRemote);

    // Date time
    time_t now = 0;
    struct tm timeinfo = { 0 };
    time(&now);
    localtime_r(&now, &timeinfo);
    
    char text[80+1];
    sprintf(text, "%4d-%2d-%2d %2d:%2d:%2d",
        /* 0*/1900+timeinfo.tm_year,
        /* 1*/timeinfo.tm_mon+1,
        /* 2*/timeinfo.tm_mday,
        /* 3*/timeinfo.tm_hour,
        /* 4*/timeinfo.tm_min,
        /* 5*/timeinfo.tm_sec);
    cJSON_AddItemToObject(pRoot, "datetime", cJSON_CreateString(text));

    STOVEMB_Give();

    char* pStr =  cJSON_PrintUnformatted(pRoot);
    cJSON_Delete(pRoot);
    return pStr;
    ERROR:
    cJSON_Delete(pRoot);
    return NULL;
}

static char* GetPairingSettingToJSON()
{ 
    cJSON* pRoot = NULL;
    pRoot = cJSON_CreateObject();
    if (pRoot == NULL)
        goto ERROR;

    size_t n = SETTINGS_ESPNOWREMOTEMAC_LEN;
    char szMacAddr[SETTINGS_ESPNOWREMOTEMAC_LEN];
    NVSJSON_GetValueString(&g_sSettingHandle, SETTINGS_EENTRY_ESPNowRemoteMac, (char*)szMacAddr, &n);
    if (n >= SETTINGS_ESPNOWREMOTEMAC_LEN)
        goto ERROR;

    cJSON_AddItemToObject(pRoot, "mac_addr", cJSON_CreateString(szMacAddr));

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
        case CHIP_ESP32C2:
            return "ESP32-C2";
        case CHIP_ESP32C3:
            return "ESP32-C3";
        case CHIP_ESP32S3:
            return "ESP32-S3";
        case CHIP_ESP32H2:
            return "ESP32-H2";
    }
    return "";
}
