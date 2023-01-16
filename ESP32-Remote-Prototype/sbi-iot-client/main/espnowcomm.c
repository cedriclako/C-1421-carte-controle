#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_now.h"
#include "espnowcomm.h"

#include "nanopb/pb_common.h"
#include "nanopb/pb_decode.h"
#include "nanopb/pb_encode.h"
#include "SBI-iot-util.h"

#define TAG "ESPNOWCOMM"

static void example_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status);
static void example_espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len);

static void SendESPNow(pb_size_t which_payload, void* pPayloadData, uint32_t u32PayloadDataLen);

static void RecvC2SStatusRespHandler(const SBI_iot_Cmd* pInCmd, const SBI_iot_S2CGetStatusResp* pC2SGetStatus);

static void wifi_uninit();
static void wifi_init(uint8_t u8CurrentChannel);

static void wifisoftap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

typedef struct
{
    bool bIsWiFiActive;

    // Timers
    TickType_t ttLastPingTicks;
    TickType_t ttScanTimeTicks;
    TickType_t ttChangeChannelTicks;
    TickType_t ttEstablishedConnectionTicks;

    bool bIsFrequencyFound;

    uint8_t u8CurrChannel;
    
    QueueHandle_t sQueueRXHandle;

    // Callback
    fnChannelFound fnChannelFoundCb;
    fnS2CGetStatusResp fnS2CGetStatusRespCb;
} SHandle;

#define ESPNOW_PMK "pmk1234567890123"

static const uint8_t m_u8BroadcastAddr[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static SHandle m_sHandle = {0};

static volatile bool m_bWaitInit = false;

static const uint8_t m_u8Magics[SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN] = SBIIOTBASEPROTOCOL_MAGIC_CMD;

void ESPNOWCOMM_Init(uint8_t u8CurrChannel)
{
    // TODO: Add frequency validation
    m_sHandle.bIsWiFiActive = false;

    m_sHandle.sQueueRXHandle = xQueueCreate(ESPNOWCOMM_QUEUERX, sizeof(ESPNOWCOMM_SMsg));

    m_sHandle.ttLastPingTicks = xTaskGetTickCount();
    m_sHandle.ttScanTimeTicks = xTaskGetTickCount();
    m_sHandle.ttChangeChannelTicks = xTaskGetTickCount();

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifisoftap_event_handler,
                                                        NULL,
                                                        NULL));

    m_sHandle.ttEstablishedConnectionTicks = xTaskGetTickCount();

    if (u8CurrChannel == 0)
    {
        m_sHandle.bIsFrequencyFound = false;
        m_sHandle.u8CurrChannel = u8CurrChannel;
    }
    else
    {
        // Go directly on the locked frequency.
        m_sHandle.bIsFrequencyFound = true;
        m_sHandle.u8CurrChannel = u8CurrChannel;
        wifi_init(u8CurrChannel);
    }
}

void ESPNOWCOMM_SetChannelFoundCallback(fnChannelFound fnChannelFoundCb)
{
    m_sHandle.fnChannelFoundCb = fnChannelFoundCb;
}

void ESPNOWCOMM_SetS2CGetStatusRespCallback(fnS2CGetStatusResp fnS2CGetStatusRespCb)
{
    m_sHandle.fnS2CGetStatusRespCb = fnS2CGetStatusRespCb;
}

static void wifi_init(uint8_t u8CurrentChannel)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP) );
    
    wifi_config_t wifi_configAP = {
        .ap = {
            .ssid = "test",
            .ssid_len = 0,
            .channel = u8CurrentChannel,
            .max_connection = 5,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_configAP));
    ESP_LOGI(TAG, "esp_wifi_start()");
    ESP_ERROR_CHECK(esp_wifi_start());
    
    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(example_espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(example_espnow_recv_cb) );

    /* Set primary master key. */
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)ESPNOW_PMK) );

    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t peer;
    memset(&peer, 0, sizeof(esp_now_peer_info_t));
    peer.channel = u8CurrentChannel;
    peer.ifidx = ESP_IF_WIFI_AP;
    peer.encrypt = false;
    memcpy(peer.peer_addr, m_u8BroadcastAddr, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(&peer) );

    m_sHandle.bIsWiFiActive = true;
}

static void wifi_uninit()
{
    ESP_ERROR_CHECK( esp_wifi_stop());
    ESP_ERROR_CHECK( esp_wifi_deinit());
    ESP_ERROR_CHECK( esp_now_deinit() );

    m_sHandle.bIsWiFiActive = false;
}

void ESPNOWCOMM_Handler()
{
    ESPNOWCOMM_SMsg msg;
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
            case SBI_iot_Cmd_s2c_get_status_resp_tag:
                RecvC2SStatusRespHandler(&inCmd, &inCmd.payload.s2c_get_status_resp);
                break;
            default:
                ESP_LOGW(TAG, "Receiving transmission, seq: %d, which: %d, len: %d", inCmd.seq_number, inCmd.which_payload, msg.u8BufferCount);
                // ESP_LOGE(TAG, "Unknown payload: %d", inCmd.which_payload);
                break;
        }
    }

    // Attempt to scan for frequency
    if (!m_sHandle.bIsFrequencyFound)
    {
        // Looking up to find the frequency by scanning channels
        if ( (xTaskGetTickCount() - m_sHandle.ttChangeChannelTicks) > pdMS_TO_TICKS(ESPNOWCOMM_SCANCHANNEL_TIMEOUTPERCHAN_MS))
        {
            // Change ESP-Now channel
            if (m_sHandle.u8CurrChannel == 0)
                m_sHandle.u8CurrChannel = 1;
            else
                m_sHandle.u8CurrChannel = (m_sHandle.u8CurrChannel + 1) % 12;

            if (m_sHandle.bIsWiFiActive)
                wifi_uninit();
            
            m_bWaitInit = false;
            wifi_init(m_sHandle.u8CurrChannel);
            // TEst
            while(!m_bWaitInit);

            ESP_LOGI(TAG, "Change channel for: %d", m_sHandle.u8CurrChannel);

            SBI_iot_C2SGetStatus c2sGetStatus;
            c2sGetStatus.has_remote_state = true;
            c2sGetStatus.remote_state.temperatureC_curr = 25;
            SendESPNow(SBI_iot_Cmd_c2s_get_status_tag, &c2sGetStatus, sizeof(SBI_iot_C2SGetStatus));

            m_sHandle.ttChangeChannelTicks = xTaskGetTickCount();
        }
    }
}

static void RecvC2SStatusRespHandler(const SBI_iot_Cmd* pInCmd, const SBI_iot_S2CGetStatusResp* pC2SGetStatus)
{
    if (!m_sHandle.bIsFrequencyFound)
    {
        m_sHandle.bIsFrequencyFound = true;
        m_sHandle.ttLastPingTicks = 0;

        if (m_sHandle.fnChannelFoundCb != NULL)
            m_sHandle.fnChannelFoundCb(m_sHandle.u8CurrChannel);
    }

    if (m_sHandle.fnS2CGetStatusRespCb != NULL)
        m_sHandle.fnS2CGetStatusRespCb(pC2SGetStatus);
}

static void SendESPNow(pb_size_t which_payload, void* pPayloadData, uint32_t u32PayloadDataLen)
{
    // Send a few probe message
    uint8_t u8OutBuffers[SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN + SBIIOTBASEPROTOCOL_MAXPAYLOADLEN];
    pb_ostream_t outputStream = pb_ostream_from_buffer(u8OutBuffers + SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN, SBIIOTBASEPROTOCOL_MAXPAYLOADLEN);
    memcpy(u8OutBuffers, m_u8Magics, SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN);

    SBI_iot_Cmd cmdResp = SBI_iot_Cmd_init_default;
    static uint32_t seq_number = 1;
    cmdResp.seq_number = seq_number++;
    cmdResp.which_payload = which_payload;
    memcpy(&cmdResp.payload, pPayloadData, u32PayloadDataLen);

    pb_encode(&outputStream, SBI_iot_Cmd_fields, &cmdResp);
    const int len = SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN + outputStream.bytes_written;

    ESP_LOGI(TAG, "==> which_payload: %d (%s), seq_number: %d, len: %d", 
        cmdResp.which_payload, SBIIOTUTIL_GetCmdPayloadPrettyString(cmdResp.which_payload), 
        cmdResp.seq_number, len);

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

    ESP_LOGI(TAG, "Sent, status: %d", (int)status);
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

    // Put into receive queue
    ESPNOWCOMM_SMsg msg;
    msg.u8BufferCount = len - SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN;

    memcpy(msg.u8Buffers, data + SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN, msg.u8BufferCount);
    xQueueSend(m_sHandle.sQueueRXHandle, &msg, 0); 
}

static void wifisoftap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_START)
    {
        m_bWaitInit = true;
        ESP_LOGI(TAG, "event_id: %d [WIFI_EVENT_AP_START]", event_id);
    }
    else if (event_id == WIFI_EVENT_AP_STOP)
    {
        ESP_LOGI(TAG, "event_id: %d [WIFI_EVENT_AP_STOP]", event_id);       
    }
    else
    {
        ESP_LOGI(TAG, "event_id: %d [WIFI_EVENT_AP_*]", event_id);   
    }
}