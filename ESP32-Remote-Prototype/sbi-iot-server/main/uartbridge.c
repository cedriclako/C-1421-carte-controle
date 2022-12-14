#include "hardwaregpio.h"
#include "uartbridge.h"
#include "uart_protocol_dec.h"
#include "esp_log.h"

#define TAG "UARTBridge"

// UART Protocol decoder handle
static UARTPROTOCOLDEC_SHandle m_sHandleDecoder;
static uint8_t m_u8UARTProtocolBuffers[255];

// Callbacks
static void AcceptFrame(const UARTPROTOCOLDEC_SHandle* psHandle, uint8_t u8ID, const uint8_t u8Payloads[], uint8_t u8PayloadLen);
static void DropFrame(const UARTPROTOCOLDEC_SHandle* psHandle, const char* szReason);
static int64_t GetTimerCountMS(const UARTPROTOCOLDEC_SHandle* psHandle);

static UARTPROTOCOLDEC_SConfig m_sConfig = 
{ 
    .u8PayloadBuffers = m_u8UARTProtocolBuffers, 
    .u8PayloadBufferLen = sizeof(m_u8UARTProtocolBuffers),

    .u32FrameReceiveTimeOutMS = 50,

    .fnAcceptFrameCb = AcceptFrame,
    .fnDropFrameCb = DropFrame,
    .fnGetTimerCountMSCb = GetTimerCountMS
};

void UARTBRIDGE_Init()
{
    UARTPROTOCOLDEC_Init(&m_sHandleDecoder, &m_sConfig);
}

void UARTBRIDGE_Handler()
{
    // Read data from the UART
    uint8_t u8UARTDriverBuffers[128];
    int len = 0;
    while ((len = uart_read_bytes(HWGPIO_BRIDGEUART_PORT_NUM, u8UARTDriverBuffers, sizeof(u8UARTDriverBuffers), 0)) != 0)
    {
        UARTPROTOCOLDEC_HandleIn(&m_sHandleDecoder, u8UARTDriverBuffers, len);
        // uart_write_bytes(HWGPIO_BRIDGEUART_PORT_NUM, (const char *) u8UARTDriverBuffers, len);
    }
}

static void AcceptFrame(const UARTPROTOCOLDEC_SHandle* psHandle, uint8_t u8ID, const uint8_t u8Payloads[], uint8_t u8PayloadLen)
{
    ESP_LOGI(TAG, "Accepted frame, ID: %d, len: %d", u8ID, u8PayloadLen);
}

static void DropFrame(const UARTPROTOCOLDEC_SHandle* psHandle, const char* szReason)
{
    ESP_LOGE(TAG, "Dropped frame: %s", szReason);
}

static int64_t GetTimerCountMS(const UARTPROTOCOLDEC_SHandle* psHandle)
{
    return esp_timer_get_time() / 1000ULL;
}