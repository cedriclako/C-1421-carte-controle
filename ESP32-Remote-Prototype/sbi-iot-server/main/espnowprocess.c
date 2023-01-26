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
    // Temperature setpoint
    bool has_temp_sp;
    SBI_iot_common_TemperatureSetPoint temp_sp;
    
    bool has_tempC_current;
    float tempC_current;
} SRemoteState;

typedef struct
{
    ESPNOWPROCESS_ESPNowInfo sESPNowInfo;

    QueueHandle_t sQueueRXHandle;
    // QueueHandle_t sQueueTXHandle;

    // Related to the remote state
    SRemoteState sRemoteState;
} SHandle;

static void example_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status);
static void example_espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len);

static uint8_t m_u8BroadcastAddr[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static const uint8_t m_u8Magics[SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN] = SBIIOTBASEPROTOCOL_MAGIC_CMD;

static void SendESPNow(pb_size_t which_payload, uint32_t seq_number, void* pPayloadData, uint32_t u32PayloadDataLen);

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
    memset(&m_sHandle.sRemoteState, 0, sizeof(SRemoteState));

    m_sHandle.sRemoteState.has_tempC_current = false;

    m_sHandle.sRemoteState.has_temp_sp = true;
    m_sHandle.sRemoteState.temp_sp.temp = 21.0f;
    m_sHandle.sRemoteState.temp_sp.unit = SBI_iot_common_ETEMPERATUREUNIT_Celcius;

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
            ESP_LOGE(TAG, "Frame recv, unable decode, len: %d", msg.u8BufferCount);
            return;
        }

        ESP_LOGI(TAG, "<== which_payload: %d (%s), seq_number: %d, len: %d",
            inCmd.which_payload, SBIIOTUTIL_GetCmdPayloadPrettyString(inCmd.which_payload),
            inCmd.seq_number, msg.u8BufferCount);

        switch(inCmd.which_payload)
        {
            case SBI_iot_Cmd_c2s_get_status_tag:
                RecvC2SStatusHandler(&inCmd, &inCmd.payload.c2s_get_status);
                break;
            case SBI_iot_Cmd_c2s_change_settingsp_tag:
                RecvC2SChangeSettingSPHandler(&inCmd, &inCmd.payload.c2s_change_settingsp);
                break;
            default:
                ESP_LOGW(TAG, "Receiving transmission, seq: %d, which: %d, len: %d", inCmd.seq_number, inCmd.which_payload, msg.u8BufferCount);
                // ESP_LOGE(TAG, "Unknown payload: %d", inCmd.which_payload);
                break;
        }
    }
}

static void RecvC2SStatusHandler(SBI_iot_Cmd* pInCmd, SBI_iot_C2SGetStatus* pC2SGetStatus)
{
    STOVEMB_Take(portMAX_DELAY);

    const STOVEMB_SMemBlock* pMB = STOVEMB_GetMemBlockRO();

    // -------------------------------------
    // Decode remote and record state
    if (pC2SGetStatus->has_remote_state)
    {
        m_sHandle.sRemoteState.has_tempC_current = true;
        m_sHandle.sRemoteState.tempC_current = pC2SGetStatus->remote_state.temperatureC_curr;
        ESP_LOGI(TAG, "remote temperatureC_curr: %.2f", pC2SGetStatus->remote_state.temperatureC_curr);
    }

    // -------------------------------------
    // Return a response
    SBI_iot_S2CGetStatusResp s2c_get_status_resp;
    s2c_get_status_resp.has_stove_state = true;
    s2c_get_status_resp.stove_state.has_fan_speed_set = true;
    s2c_get_status_resp.stove_state.fan_speed_set.is_automatic = true;
    s2c_get_status_resp.stove_state.fan_speed_set.curr = 1;

    s2c_get_status_resp.stove_state.has_fan_speed_boundary = true;
    s2c_get_status_resp.stove_state.fan_speed_boundary.min = 1;
    s2c_get_status_resp.stove_state.fan_speed_boundary.max = 4;

    s2c_get_status_resp.stove_state.is_open_air = false;

    // These values comes from the remote
    if (m_sHandle.sRemoteState.has_temp_sp)
    {
        s2c_get_status_resp.stove_state.has_remote_temperature_setp = true;
        s2c_get_status_resp.stove_state.remote_temperature_setp.unit = m_sHandle.sRemoteState.temp_sp.unit;
        s2c_get_status_resp.stove_state.remote_temperature_setp.temp = m_sHandle.sRemoteState.temp_sp.temp;
    }

    if (pMB->sS2CReqVersionRespIsSet)
    {
        s2c_get_status_resp.has_stove_info = true;
        s2c_get_status_resp.stove_info.device_type = SBI_iot_EDEVICETYPE_Stove_V1;
        s2c_get_status_resp.stove_info.has_sw_version = true;
        s2c_get_status_resp.stove_info.sw_version.major = pMB->sS2CReqVersionResp.sVersion.u8Major;
        s2c_get_status_resp.stove_info.sw_version.minor = pMB->sS2CReqVersionResp.sVersion.u8Minor;
        s2c_get_status_resp.stove_info.sw_version.revision = pMB->sS2CReqVersionResp.sVersion.u8Revision;
    }

    //if (FillBridgeInfo(&s2c_get_status_resp.bridge_info))
    //    s2c_get_status_resp.has_bridge_info = true;

    // Date time
    s2c_get_status_resp.stove_state.has_datetime = true;
    s2c_get_status_resp.stove_state.datetime.has_date = true;
    s2c_get_status_resp.stove_state.datetime.date.year = 2022;
    s2c_get_status_resp.stove_state.datetime.date.month = 6;
    s2c_get_status_resp.stove_state.datetime.date.day = 21;
    s2c_get_status_resp.stove_state.datetime.has_time = true;
    s2c_get_status_resp.stove_state.datetime.time.hour = 17;
    s2c_get_status_resp.stove_state.datetime.time.hour = 30;
    s2c_get_status_resp.stove_state.datetime.time.hour = 21;

    SendESPNow(SBI_iot_Cmd_s2c_get_status_resp_tag, pInCmd->seq_number, &s2c_get_status_resp, sizeof(SBI_iot_S2CGetStatusResp));

    STOVEMB_Give();
}

static void RecvC2SChangeSettingSPHandler(SBI_iot_Cmd* pInCmd, SBI_iot_C2SChangeSettingSP* pC2SChangeSettingSP)
{
    ESP_LOGI(TAG, "C2SChangeSettingSP, has_temperature_setp: %s, temperature_setp: %.2f", 
        (pC2SChangeSettingSP->has_temperature_setp ? "true" : "false"), 
        pC2SChangeSettingSP->temperature_setp.temp);

    if (pC2SChangeSettingSP->has_temperature_setp)
    {
        m_sHandle.sRemoteState.has_temp_sp = true;
        m_sHandle.sRemoteState.temp_sp.temp = pC2SChangeSettingSP->temperature_setp.temp;
        m_sHandle.sRemoteState.temp_sp.unit = pC2SChangeSettingSP->temperature_setp.unit;
    }
}
/*
static bool FillBridgeInfo(SBI_iot_DeviceInfo* pDeviceInfo)
{
    pDeviceInfo->device_type = SBI_iot_EDEVICETYPE_EDEVICETYPE_IoTServer_V1;
    pDeviceInfo->has_sw_version = true;
    pDeviceInfo->sw_version.major = VERSION_MAJOR;
    pDeviceInfo->sw_version.minor = VERSION_MINOR;
    pDeviceInfo->sw_version.revision = VERSION_REVISION;
    return true;
}*/

ESPNOWPROCESS_ESPNowInfo ESPNOWPROCESS_GetESPNowInfo()
{
    return m_sHandle.sESPNowInfo;
}

static void SendESPNow(pb_size_t which_payload, uint32_t seq_number, void* pPayloadData, uint32_t u32PayloadDataLen)
{
    // Send a few probe message
    uint8_t u8OutBuffers[SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN + SBIIOTBASEPROTOCOL_MAXPAYLOADLEN];
    pb_ostream_t outputStream = pb_ostream_from_buffer(u8OutBuffers + SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN, SBIIOTBASEPROTOCOL_MAXPAYLOADLEN);
    memcpy(u8OutBuffers, m_u8Magics, SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN);

    SBI_iot_Cmd cmdResp = SBI_iot_Cmd_init_default;
    cmdResp.seq_number = 1;
    cmdResp.which_payload = which_payload;
    memcpy(&cmdResp.payload, pPayloadData, u32PayloadDataLen);

    pb_encode(&outputStream, SBI_iot_Cmd_fields, &cmdResp);
    const int len = SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN + outputStream.bytes_written;

    ESP_LOGI(TAG, "==> which_payload: %d (%s), seq_number: %d, len: %d",
        which_payload, SBIIOTUTIL_GetCmdPayloadPrettyString(which_payload),
        seq_number, len);

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
        ESP_LOGE(TAG, "dropped RX, too big payload, len: %d", len);
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
