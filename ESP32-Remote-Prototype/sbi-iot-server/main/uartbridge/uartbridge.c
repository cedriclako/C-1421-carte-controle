#include "hardwaregpio.h"
#include "uartbridge.h"
#include "uart_protocol_dec.h"
#include "uart_protocol_enc.h"
#include "esp_log.h"
#include "event.h"

#define TAG "UARTBridge"

// UART Protocol decoder handle
static UARTPROTOCOLDEC_SHandle m_sHandleDecoder;
static uint8_t m_u8UARTProtocolBuffers[255];

static UARTPROTOCOLENC_SHandle m_sHandleEncoder;

// Callbacks
static void DecAcceptFrame(const UARTPROTOCOLDEC_SHandle* psHandle, uint8_t u8ID, const uint8_t u8Payloads[], uint16_t u16PayloadLen);
static void DecDropFrame(const UARTPROTOCOLDEC_SHandle* psHandle, const char* szReason);
static int64_t GetTimerCountMS(const UARTPROTOCOLDEC_SHandle* psHandle);

static void EncWriteUART(const UARTPROTOCOLENC_SHandle* psHandle, const uint8_t u8Datas[], uint32_t u32DataLen);

// Event loops
static void RequestConfigReloadEvent(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

static UARTPROTOCOLDEC_SConfig m_sConfigDecoder = 
{ 
    .u8PayloadBuffers = m_u8UARTProtocolBuffers, 
    .u16PayloadBufferLen = sizeof(m_u8UARTProtocolBuffers),

    .u32FrameReceiveTimeOutMS = 50,

    // Callbacks
    .fnAcceptFrameCb = DecAcceptFrame,
    .fnDropFrameCb = DecDropFrame,
    .fnGetTimerCountMSCb = GetTimerCountMS
};

static UARTPROTOCOLENC_SConfig m_sConfigEncoder = 
{
    // Callbacks
    .fnWriteCb = EncWriteUART
};

void UARTBRIDGE_Init()
{
    // Decoder
    UARTPROTOCOLDEC_Init(&m_sHandleDecoder, &m_sConfigDecoder);
    // Encoder
    UARTPROTOCOLENC_Init(&m_sHandleEncoder, &m_sConfigEncoder);

    // Register events
    esp_event_handler_register_with(EVENT_g_LoopHandle, MAINAPP_EVENT, REQUESTCONFIGRELOAD_EVENT, RequestConfigReloadEvent, NULL);
}

void UARTBRIDGE_Handler()
{
    // Read data from the UART
    uint8_t u8UARTDriverBuffers[128];
    int len = 0;
    while ((len = uart_read_bytes(HWGPIO_BRIDGEUART_PORT_NUM, u8UARTDriverBuffers, sizeof(u8UARTDriverBuffers), 0)) != 0)
    {
        UARTPROTOCOLDEC_HandleIn(&m_sHandleDecoder, u8UARTDriverBuffers, len);
    }
}

static int64_t GetTimerCountMS(const UARTPROTOCOLDEC_SHandle* psHandle)
{
    return esp_timer_get_time() / 1000ULL;
}

static void EncWriteUART(const UARTPROTOCOLENC_SHandle* psHandle, const uint8_t u8Datas[], uint32_t u32DataLen)
{
    uart_write_bytes(HWGPIO_BRIDGEUART_PORT_NUM, u8Datas, u32DataLen);
}

static void DecAcceptFrame(const UARTPROTOCOLDEC_SHandle* psHandle, uint8_t u8ID, const uint8_t u8Payloads[], uint16_t u16PayloadLen)
{
    ESP_LOGI(TAG, "Accepted frame, ID: %d, len: %d", u8ID, u16PayloadLen);
    // esp_event_post_to(EVENT_g_LoopHandle, MAINAPP_EVENT, REQUESTCONFIGRELOAD_EVENT, NULL, 0, 0);
}

static void DecDropFrame(const UARTPROTOCOLDEC_SHandle* psHandle, const char* szReason)
{
    // Exists mostly for debug purpose
    ESP_LOGE(TAG, "Dropped frame: %s", szReason);
}

void UARTBRIDGE_SendFrame(UFEC23PROTOCOL_FRAMEID eFrameID, uint8_t u8Payloads[], uint16_t u16PayloadLen)
{
    UARTPROTOCOLENC_Send(&m_sHandleEncoder, (uint8_t)eFrameID, u8Payloads, u16PayloadLen);
}

static void RequestConfigReloadEvent(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "RequestConfigReloadEvent");
}