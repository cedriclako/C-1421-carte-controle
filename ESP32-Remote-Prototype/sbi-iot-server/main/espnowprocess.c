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
static void example_espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len);

static uint8_t m_u8BroadcastAddr[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static const uint8_t m_u8Magics[SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN] = SBIIOTBASEPROTOCOL_MAGIC_CMD;

static void SendESPNow(pb_size_t which_payload, uint32_t transaction_id, void* pPayloadData, uint32_t u32PayloadDataLen);

// static bool FillBridgeInfo(SBI_iot_DeviceInfo* pDeviceInfo);

// Receivers
static void RecvC2SStatusHandler(SBI_iot_Cmd* pInCmd, SBI_iot_C2SGetStatus* pC2SGetStatus);
static void RecvC2SChangeSettingSPHandler(SBI_iot_Cmd* pInCmd, SBI_iot_C2SChangeSettingSP* pC2SChangeSettingSP);

// =====================
// ESP-NOW
// static xQueueHandle s_example_espnow_queue;
static SHandle m_sHandle;

void ESPNOWPROCESS_Init()
{
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
    if (mac_addr == NULL) {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }

    m_sHandle.sESPNowInfo.u32TX++;
}

static void example_espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    if (mac_addr == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    if (len > SBIIOTBASEPROTOCOL_MAXPAYLOADLEN)
    {
        ESP_LOGE(TAG, "dropped RX, too big payload, len: %"PRId32, (int32_t)len);
        return;
    }

    if (len < SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN)
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
    msg.u8BufferCount = len - SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN;
    memcpy(msg.u8SrcMACs, mac_addr, ESP_NOW_ETH_ALEN);
    memcpy(msg.u8Buffers, data + SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN, msg.u8BufferCount);
    xQueueSend(m_sHandle.sQueueRXHandle, &msg, 0);
}
