#include <time.h>
#include <inttypes.h>

#include "espnowprocess.h"
#include "esp_log.h"

#include "SBI.iot.pb.h"
#include "SBI.iot.common.pb.h"
#include "SBI-iot-util.h"

#include "nanopb/pb_common.h"
#include "nanopb/pb_decode.h"
#include "nanopb/pb_encode.h"

#include "uartbridge/stovemb.h"
#include "version.h"
#include "uartbridge/uartbridge.h"

#if SBI_CL
#include "cJSON.h"
#include "settings.h"
#include "nvsjson.h"
#endif

#define TAG "espnowprocess"

#define CONFIG_ESPNOW_PMK "pmk1234567890123"

typedef struct
{
    ESPNOWPROCESS_ESPNowInfo sESPNowInfo;

    // Sequence number
    uint32_t u32SequenceNumber;

    // Ignore duplicated ...
    int32_t s32LastTransactionWhichPayload;
    uint32_t u32LastTransactionId;

    QueueHandle_t sQueueRXHandle;
    // QueueHandle_t sQueueTXHandle;
} SHandle;



static void example_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status);
static void example_espnow_recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len);

uint8_t m_u8BroadcastAddr[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static const uint8_t m_u8Magics[SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN] = SBIIOTBASEPROTOCOL_MAGIC_CMD;

static void SendESPNow(pb_size_t which_payload, uint32_t transaction_id, void* pPayloadData, uint32_t u32PayloadDataLen);

#if SBI_CL
static void sendDataToRemote(ESPNOWDEBUG_SMsg data);
ESPNOWRMT_SMsg espNowDataRcv;
ESPNOWDEBUG_SMsg dataDebug;
#endif
// static bool FillBridgeInfo(SBI_iot_DeviceInfo* pDeviceInfo);

// Receivers
static void RecvC2SStatusHandler(SBI_iot_Cmd* pInCmd, SBI_iot_C2SGetStatus* pC2SGetStatus);
static void RecvC2SChangeSettingSPHandler(SBI_iot_Cmd* pInCmd, SBI_iot_C2SChangeSettingSP* pC2SChangeSettingSP);

// =====================
// ESP-NOW
// static xQueueHandle s_example_espnow_queue;
static SHandle m_sHandle;
#if SBI_CL
extern NVSJSON_SHandle g_sSettingHandle;
#endif

void ESPNOWPROCESS_Init()
{
#if SBI_CL
    size_t n = SETTINGS_ESPNOWREMOTEMAC_LEN;
    char szMacAddr[SETTINGS_ESPNOWREMOTEMAC_LEN];

    memset(szMacAddr, '\0', sizeof(szMacAddr));
    
    NVSJSON_GetValueString(&g_sSettingHandle, SETTINGS_EENTRY_ESPNowRemoteMac, (char*)szMacAddr, &n);

    //strcpy(szMacAddr, "d4:d4:da:5c:80:bd"); //quick test

    memset(dataDebug.macAddr, '\0', sizeof(dataDebug.macAddr));
    memcpy(dataDebug.macAddr,szMacAddr, sizeof(dataDebug.macAddr));
    
    if((strlen(szMacAddr) >= 17) && (strcmp(szMacAddr, "00:00:00:00:00:00")))
    {
        sscanf(szMacAddr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",  &m_u8BroadcastAddr[0], &m_u8BroadcastAddr[1], &m_u8BroadcastAddr[2], &m_u8BroadcastAddr[3], &m_u8BroadcastAddr[4], &m_u8BroadcastAddr[5] );
    }
#endif
    // Default values
    memset(&m_sHandle.sESPNowInfo, 0, sizeof(ESPNOWPROCESS_ESPNowInfo));

    m_sHandle.sQueueRXHandle = xQueueCreate(ESPNOWPROCESS_QUEUERX, sizeof(ESPNOWPROCESS_SMsg));

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(example_espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(example_espnow_recv_cb) );

    /* Set primary master key. */
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t sPeer;
    memset(&sPeer, 0, sizeof(esp_now_peer_info_t));
    sPeer.channel = 0;
    sPeer.ifidx = WIFI_IF_AP;
    sPeer.encrypt = false;
    memcpy(sPeer.peer_addr, m_u8BroadcastAddr, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(&sPeer) );

    // Add encrypted clients

    
}

void ESPNOWPROCESS_Handler()
{

#if SBI_CL
    ESPNOWPROCESS_SMsg msg;
    if (xQueueReceive(m_sHandle.sQueueRXHandle, &msg, 0) == pdTRUE)
    {
        STOVEMB_Take(portMAX_DELAY);
    
        STOVEMB_SMemBlock* pMB = STOVEMB_GetMemBlock();

        cJSON *espnowdebug_json = cJSON_Parse(pMB->szDebugJSONString);
        printf("espnowdebug_json: %s\n\r", pMB->szDebugJSONString);
        if (espnowdebug_json == NULL)
        {
            ESP_LOGI(TAG, "this file is not a json ...");
        }
        else
        {
            if(cJSON_IsString(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "#")))
            {
                memset(dataDebug.time, '\0', sizeof(dataDebug.time)); 
                strcpy(dataDebug.time, cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "#")->valuestring);
            }
            if(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "Tbaffle")))
            {
                dataDebug.Tbaffle = cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "Tbaffle")->valuedouble;
            }
            if(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "Tavant")))
            {
                dataDebug.Tavant = cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "Tavant")->valuedouble;
            }
            if(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "Plenum")))
            {
                dataDebug.Plenum = cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "Plenum")->valuedouble;
            }
            if(cJSON_IsString(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "State")))
            {
                memset(dataDebug.State, '\0', sizeof(dataDebug.State)); 
                strcpy(dataDebug.State, cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "State")->valuestring);
            }
            if(cJSON_IsString(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "tStat")))
            {
                memset(dataDebug.tStat, '\0', sizeof(dataDebug.tStat)); 
                strcpy(dataDebug.tStat, cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "tStat")->valuestring);
            }
            if(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "dTbaffle")))
            {
                dataDebug.dTbaffle = cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "dTbaffle")->valuedouble;
            }
            if(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "FanSpeed")))
            {
                dataDebug.FanSpeed = cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "FanSpeed")->valueint;
            }
            if(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "Grille")))
            {
                dataDebug.Grille = cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "Grille")->valueint;
            }
            if(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "Prim")))
            {
                dataDebug.Prim = cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "Prim")->valueint;
            }
            if(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "Sec")))
            {
                dataDebug.Sec = cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "Sec")->valueint;
            }
            if(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "Tboard")))
            {
                dataDebug.Tboard = cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "Tboard")->valuedouble;
            }
            if(cJSON_IsString(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "Door")))
            {
                memset(dataDebug.Door, '\0', sizeof(dataDebug.Door)); 
                strcpy(dataDebug.Door, cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "Door")->valuestring);
            }
            if(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "PartCH0ON")))
            {
                dataDebug.PartCH0ON = cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "PartCH0ON")->valueint;
            }
            if(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "PartCH1ON")))
            {
                dataDebug.PartCH1ON = cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "PartCH1ON")->valueint;
            }
            if(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "PartCH0OFF")))
            {
                dataDebug.PartCH0OFF = cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "PartCH0OFF")->valueint;
            }
            if(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "PartCH1OFF")))
            {
                dataDebug.PartCH1OFF = cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "PartCH1OFF")->valueint;
            }
            if(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "PartVar")))
            {
                dataDebug.PartVar = cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "PartVar")->valueint;
            }
            if(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "PartSlope")))
            {
                dataDebug.PartSlope = cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "PartSlope")->valuedouble;
            }
            if(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "TPart")))
            {
                dataDebug.TPart = cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "TPart")->valueint;
            }
            if(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "PartCurr")))
            {
                dataDebug.PartCurr = cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "PartCurr")->valuedouble;
            }
            if(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "PartLuxON")))
            {
                dataDebug.PartLuxON = cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "PartLuxON")->valueint;
            }
            if(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "PartLuxOFF")))
            {
                dataDebug.PartLuxOFF = cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "PartLuxOFF")->valueint;
            }
            if(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "PartTime")))
            {
                dataDebug.PartTime = cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "PartTime")->valueint;
            }
            if(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "dTavant")))
            {
                dataDebug.dTavant = cJSON_GetObjectItemCaseSensitive(espnowdebug_json, "dTavant")->valuedouble;
            }
        }
        cJSON_Delete(espnowdebug_json);

        sendDataToRemote(dataDebug);

        STOVEMB_Give();
    }
#else
    ESPNOWPROCESS_SMsg msg;
    if (xQueueReceive(m_sHandle.sQueueRXHandle, &msg, 0) == pdTRUE)
    {
        pb_istream_t recvStream = pb_istream_from_buffer(msg.u8Buffers, msg.u8BufferCount);

        SBI_iot_Cmd inCmd = SBI_iot_Cmd_init_default;
        if (!pb_decode(&recvStream, SBI_iot_Cmd_fields, &inCmd))
        {
            ESP_LOGE(TAG, "Frame recv, unable decode, len: %"PRId32, (int32_t)msg.u8BufferCount);
            return;
        }

        ESP_LOGI(TAG, "<== which_payload: %"PRId32" (%s), seq_number: %"PRId32", len: %"PRId32,
            (int32_t)inCmd.which_payload, SBIIOTUTIL_GetCmdPayloadPrettyString(inCmd.which_payload),
            (int32_t)inCmd.seq_number, (int32_t)msg.u8BufferCount);

        // Last transaction ID to discriminate against packet sent multiple time.
        // If the number is 0, we ignore it.
        if (inCmd.transaction_id != 0)
        {
            if (m_sHandle.s32LastTransactionWhichPayload == inCmd.which_payload &&
                m_sHandle.u32LastTransactionId == inCmd.transaction_id)
            {
                ESP_LOGW(TAG, "Frame recv dropped [duplicated], payloadid: %"PRId32", transactionid: %"PRId32, 
                    (int32_t)inCmd.which_payload, (int32_t)inCmd.transaction_id);
                return;
            }
            m_sHandle.s32LastTransactionWhichPayload = inCmd.which_payload;
            m_sHandle.u32LastTransactionId = inCmd.transaction_id;
        }

        switch(inCmd.which_payload)
        {
            case SBI_iot_Cmd_c2s_get_status_tag:
                RecvC2SStatusHandler(&inCmd, &inCmd.payload.c2s_get_status);
                break;
            case SBI_iot_Cmd_c2s_change_settingsp_tag:
                RecvC2SChangeSettingSPHandler(&inCmd, &inCmd.payload.c2s_change_settingsp);
                break;
            default:
                ESP_LOGW(TAG, "Receiving transmission, seq: %"PRId32", which: %"PRId32", len: %"PRId32, (int32_t)inCmd.seq_number, (int32_t)inCmd.which_payload, (int32_t)msg.u8BufferCount);
                // ESP_LOGE(TAG, "Unknown payload: %d", inCmd.which_payload);
                break;
        }
    }
#endif
}

static void RecvC2SStatusHandler(SBI_iot_Cmd* pInCmd, SBI_iot_C2SGetStatus* pC2SGetStatus)
{
    STOVEMB_Take(portMAX_DELAY);

    STOVEMB_SMemBlock* pMB = STOVEMB_GetMemBlock();

    // -------------------------------------
    // Decode remote and record state
    if (pC2SGetStatus->has_remote_state)
    {
        pMB->sRemoteData.bHasTempCurrentC = true;
        pMB->sRemoteData.fTempCurrentC = pC2SGetStatus->remote_state.temperatureC_curr;
        ESP_LOGI(TAG, "remote temperatureC_curr: %.2f", pC2SGetStatus->remote_state.temperatureC_curr);
    }

    // -------------------------------------
    // Return a response
    SBI_iot_S2CGetStatusResp resp = {0};
    resp.has_stove_state = true;

    if (pMB->sRemoteData.bHasFanSpeed)
    {
        resp.stove_state.has_fan_speed_set = true;
        resp.stove_state.fan_speed_set.curr = pMB->sRemoteData.eFanSpeedCurr;
    }

    // These values comes from the remote
    if (pMB->sRemoteData.bHasTempSetPoint)
    {
        resp.stove_state.has_remote_temperature_setp = true;
        resp.stove_state.remote_temperature_setp.unit = pMB->sRemoteData.sTempSetpoint.unit;
        resp.stove_state.remote_temperature_setp.temp = pMB->sRemoteData.sTempSetpoint.temp;
    }

    // Return stove related informations
    if (pMB->sS2CReqVersionRespIsSet)
    {
        resp.has_stove_info = true;
        resp.stove_info.device_type = SBI_iot_EDEVICETYPE_Stove_V1;
        resp.stove_info.has_sw_version = true;
        resp.stove_info.sw_version.major = pMB->sS2CReqVersionResp.sVersion.u8Major;
        resp.stove_info.sw_version.minor = pMB->sS2CReqVersionResp.sVersion.u8Minor;
        resp.stove_info.sw_version.revision = pMB->sS2CReqVersionResp.sVersion.u8Revision;
    }

    pMB->sRemoteData.ttLastCommunicationTicks = xTaskGetTickCount();

    // Date time
    time_t now = 0;
    struct tm timeinfo = { 0 };
    time(&now);
    localtime_r(&now, &timeinfo);
    
    resp.stove_state.has_datetime = true;
    resp.stove_state.datetime.has_date = true;
    resp.stove_state.datetime.date.year = 1900+timeinfo.tm_year;
    resp.stove_state.datetime.date.month = timeinfo.tm_mon+1;
    resp.stove_state.datetime.date.day = timeinfo.tm_mday;
    resp.stove_state.datetime.has_time = true;
    resp.stove_state.datetime.time.hour = timeinfo.tm_hour;
    resp.stove_state.datetime.time.min = timeinfo.tm_min;
    resp.stove_state.datetime.time.sec = timeinfo.tm_sec;

    STOVEMB_Give();

    SendESPNow(SBI_iot_Cmd_s2c_get_status_resp_tag, pInCmd->transaction_id, &resp, sizeof(SBI_iot_S2CGetStatusResp));
}

static void RecvC2SChangeSettingSPHandler(SBI_iot_Cmd* pInCmd, SBI_iot_C2SChangeSettingSP* pC2SChangeSettingSP)
{
    STOVEMB_Take(portMAX_DELAY);
    STOVEMB_SMemBlock* pMB = STOVEMB_GetMemBlock();
    
    if (pC2SChangeSettingSP->has_temperature_setp)
    {
        pMB->sRemoteData.bHasTempSetPoint = true;
        pMB->sRemoteData.sTempSetpoint.temp = pC2SChangeSettingSP->temperature_setp.temp;
        pMB->sRemoteData.sTempSetpoint.unit = pC2SChangeSettingSP->temperature_setp.unit;
        
        ESP_LOGI(TAG, "C2SChangeSettingSP, temperature_setp: %.2f", 
            pC2SChangeSettingSP->temperature_setp.temp);
    }

    if (pC2SChangeSettingSP->has_fan_speed_set)
    {
        pMB->sRemoteData.bHasFanSpeed = true;
        int32_t s32CurrFanSpeedValue = (int32_t)pC2SChangeSettingSP->fan_speed_set.curr;
        if (s32CurrFanSpeedValue < (int32_t)SBI_iot_common_EFANSPEED_Off)
            s32CurrFanSpeedValue = (int32_t)SBI_iot_common_EFANSPEED_Off;
        else if (s32CurrFanSpeedValue >= (int32_t)SBI_iot_common_EFANSPEED_Count)
            s32CurrFanSpeedValue = (int32_t)SBI_iot_common_EFANSPEED_Count;
        pMB->sRemoteData.eFanSpeedCurr = (SBI_iot_common_EFANSPEED)s32CurrFanSpeedValue;
        ESP_LOGI(TAG, "C2SChangeSettingSP fanspeed, received: %"PRId32", set: %"PRId32, (int32_t)pC2SChangeSettingSP->fan_speed_set.curr, (int32_t)s32CurrFanSpeedValue);
    }

    ESP_LOGI(TAG, "transaction id: %"PRId32, (int32_t)pInCmd->transaction_id);

    SBI_iot_S2CChangeSettingSPResp resp;
    
    SendESPNow(SBI_iot_Cmd_s2c_change_settingsp_resp_tag, pInCmd->transaction_id, &resp, sizeof(SBI_iot_S2CChangeSettingSPResp));

    STOVEMB_Give();
}

ESPNOWPROCESS_ESPNowInfo ESPNOWPROCESS_GetESPNowInfo()
{
    return m_sHandle.sESPNowInfo;
}

static void SendESPNow(pb_size_t which_payload, uint32_t transaction_id, void* pPayloadData, uint32_t u32PayloadDataLen)
{
    // Send a few probe message
    uint8_t u8OutBuffers[SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN + SBIIOTBASEPROTOCOL_MAXPAYLOADLEN];
    pb_ostream_t outputStream = pb_ostream_from_buffer(u8OutBuffers + SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN, SBIIOTBASEPROTOCOL_MAXPAYLOADLEN);
    memcpy(u8OutBuffers, m_u8Magics, SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN);

    SBI_iot_Cmd cmdResp = SBI_iot_Cmd_init_default;
    cmdResp.seq_number = ++m_sHandle.u32SequenceNumber;
    cmdResp.transaction_id = transaction_id;
    cmdResp.which_payload = which_payload;
    memcpy(&cmdResp.payload, pPayloadData, u32PayloadDataLen);

    pb_encode(&outputStream, SBI_iot_Cmd_fields, &cmdResp);
    const int len = SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN + outputStream.bytes_written;

    ESP_LOGI(TAG, "==> which_payload: %"PRId32" (%s), seq_number: %"PRId32", len: %"PRId32,
        (int32_t)which_payload, SBIIOTUTIL_GetCmdPayloadPrettyString(which_payload),
        (int32_t)cmdResp.seq_number, (int32_t)len);

    esp_now_send(m_u8BroadcastAddr, u8OutBuffers, len);
}

/* ESPNOW sending or receiving callback function is called in WiFi task.
 * Users should not do lengthy operations from this task. Instead, post
 * necessary data to a queue and handle it from a lower priority task. */
static void example_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
#if SBI_CL
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    ESP_LOGI(TAG, "Last Packet Sent to: %s", macStr);
    ESP_LOGI(TAG, "Last Packet Send Status: %s", status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
#else
    if (mac_addr == NULL) {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }

    m_sHandle.sESPNowInfo.u32TX++;
#endif
}

static void example_espnow_recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len)
{
#if SBI_CL
    char macStr[18];
    char m_arrBroadcastAddr[18];
    ESPNOWRMT_SMsg dataRcvRmt;

    ESP_LOGI(TAG, "Length Packet Recv is: %d", data_len);
    
    memcpy(&dataRcvRmt, data, sizeof(dataRcvRmt));

    memset(macStr, '\0', sizeof(macStr));
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", dataRcvRmt.macAddr[0], dataRcvRmt.macAddr[1], dataRcvRmt.macAddr[2], dataRcvRmt.macAddr[3], dataRcvRmt.macAddr[4], dataRcvRmt.macAddr[5]);
    ESP_LOGI(TAG, "dataRcvRmt.macAddr: %s", macStr);
    
    memset(m_arrBroadcastAddr, '\0', sizeof(m_arrBroadcastAddr));
    snprintf(m_arrBroadcastAddr, sizeof(m_arrBroadcastAddr), "%02x:%02x:%02x:%02x:%02x:%02x", m_u8BroadcastAddr[0], m_u8BroadcastAddr[1], m_u8BroadcastAddr[2], m_u8BroadcastAddr[3], m_u8BroadcastAddr[4], m_u8BroadcastAddr[5]);
    ESP_LOGI(TAG, "m_arrBroadcastAddr: %s", m_arrBroadcastAddr);
    
    
    if(strcmp(m_arrBroadcastAddr, macStr) == 0)
    {
        espNowDataRcv = dataRcvRmt;
        ESP_LOGI(TAG, "espNowDataRcv.macAddr: %s", macStr);
        ESP_LOGI(TAG, "espNowDataRcv.tStatRmt: %d", espNowDataRcv.tStatRmt);
        ESP_LOGI(TAG, "espNowDataRcv.blowerSpeedRmt: %d", espNowDataRcv.blowerSpeedRmt);
        ESP_LOGI(TAG, "espNowDataRcv.distribSpeedRmt: %d", espNowDataRcv.distribSpeedRmt);
        ESP_LOGI(TAG, "espNowDataRcv.boostStatRmt: %d", espNowDataRcv.boostStatRmt);

        SendFrameInt32Value(UFEC23PROTOCOL_FRAMEID_StatRmt, (int32_t) espNowDataRcv.tStatRmt);
        SendFrameInt32Value(UFEC23PROTOCOL_FRAMEID_LowerSpeedRmt, (int32_t) espNowDataRcv.blowerSpeedRmt);
        SendFrameInt32Value(UFEC23PROTOCOL_FRAMEID_DistribSpeedRmt, (int32_t) espNowDataRcv.distribSpeedRmt);
        SendFrameInt32Value(UFEC23PROTOCOL_FRAMEID_BoostStatRmt, (int32_t) espNowDataRcv.boostStatRmt);

        // Put into receive queue
        ESPNOWPROCESS_SMsg msg;
        msg.u8BufferCount = data_len - SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN;
        memcpy(msg.u8SrcMACs, esp_now_info->src_addr, ESP_NOW_ETH_ALEN);
        memcpy(msg.u8Buffers, data + SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN, msg.u8BufferCount);
        xQueueSend(m_sHandle.sQueueRXHandle, &msg, 0);
    }

#else
    if (esp_now_info == NULL || data == NULL || data_len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    if (data_len > SBIIOTBASEPROTOCOL_MAXPAYLOADLEN)
    {
        ESP_LOGE(TAG, "dropped RX, too big payload, len: %"PRId32, (int32_t)data_len);
        return;
    }

    if (data_len < SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN)
    {
        ESP_LOGE(TAG, "dropped RX, no magic");
        return;
    }

    const uint8_t u8MagicComps[] = SBIIOTBASEPROTOCOL_MAGIC_CMD;
    if (memcmp(data, u8MagicComps, SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN) != 0)
    {
        ESP_LOGE(TAG, "dropped RX, invalid magic");
        return;
    }

    m_sHandle.sESPNowInfo.u32RX++;

    // Put into receive queue
    ESPNOWPROCESS_SMsg msg;
    msg.u8BufferCount = data_len - SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN;
    memcpy(msg.u8SrcMACs, esp_now_info->src_addr, ESP_NOW_ETH_ALEN);
    memcpy(msg.u8Buffers, data + SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN, msg.u8BufferCount);
    xQueueSend(m_sHandle.sQueueRXHandle, &msg, 0);
#endif
}

#if SBI_CL

static void sendDataToRemote(ESPNOWDEBUG_SMsg data) {

    esp_err_t result = esp_now_send(m_u8BroadcastAddr, (const uint8_t *) &data, sizeof(data));
   
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "Send Status: Success");
    } else if (result == ESP_ERR_ESPNOW_NOT_INIT) {
        // How did we get so far!!
        ESP_LOGI(TAG, "Send Status: ESPNOW not Init.");
    } else if (result == ESP_ERR_ESPNOW_ARG) {
        ESP_LOGI(TAG, "Send Status: Invalid Argument");
    } else if (result == ESP_ERR_ESPNOW_INTERNAL) {
        ESP_LOGI(TAG, "Send Status: Internal Error");
    } else if (result == ESP_ERR_ESPNOW_NO_MEM) {
        ESP_LOGI(TAG, "Send Status: ESP_ERR_ESPNOW_NO_MEM");
    } else if (result == ESP_ERR_ESPNOW_NOT_FOUND) {
        ESP_LOGI(TAG, "Send Status: Peer not found.");
    } else {
        ESP_LOGI(TAG, "Not sure what happened");
    }
}
#endif