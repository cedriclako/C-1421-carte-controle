#include "uart_protocol_dec.h"
#include "uart_protocol_enc.h"
#include "ufec23_protocol.h"
#include "ufec23_endec.h"
#include "esp_log.h"
#include "event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hardwaregpio.h"
#include "uartbridge.h"
#include "stovemb.h"

#define TAG "UARTBridge"

typedef enum
{
    EPROCPARAMETERENTRY_None = 0,
    EPROCPARAMETERENTRY_WaitParameterResponse = 1
} EPROCPARAMETERENTRY;

typedef struct 
{
    bool bIsConnected;

    // Connection 
    TickType_t ttLastCommTicks;

    // Parameter file download
    EPROCPARAMETERENTRY eProcParameterEntry;
    TickType_t ttParameterStartDownTicks;
} SStateMachine;

// UART Protocol decoder handle
static UARTPROTOCOLDEC_SHandle m_sHandleDecoder;
// Big buffer because we want to be able to download JSON file.
#define SENDPROTOCOL_COUNT 64

static uint8_t m_u8UARTProtocolBuffers[1024*4];
static uint8_t m_u8UARTSendProtocols[SENDPROTOCOL_COUNT];

static UARTPROTOCOLENC_SHandle m_sHandleEncoder;

// Callbacks
static void DecAcceptFrame(const UARTPROTOCOLDEC_SHandle* psHandle, uint8_t u8ID, const uint8_t u8Payloads[], uint16_t u16PayloadLen);
static void DecDropFrame(const UARTPROTOCOLDEC_SHandle* psHandle, const char* szReason);
static int64_t GetTimerCountMS(const UARTPROTOCOLDEC_SHandle* psHandle);

static void EncWriteUART(const UARTPROTOCOLENC_SHandle* psHandle, const uint8_t u8Datas[], uint32_t u32DataLen);

// Event loops
static void RequestConfigReloadEvent(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

static void ManageServerConnection();

// Events
static void ServerConnected();
static void ServerDisconnected();

static void ProcParameterStartDownload();
static void ProcParameterAbortDownload();

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

static SStateMachine m_sStateMachine;

void UARTBRIDGE_Init()
{
    STOVEMB_Init();

    // Reset communication
    m_sStateMachine.bIsConnected = false;
    m_sStateMachine.ttLastCommTicks = 0;

    // Decoder
    UARTPROTOCOLDEC_Init(&m_sHandleDecoder, &m_sConfigDecoder);
    // Encoder
    UARTPROTOCOLENC_Init(&m_sHandleEncoder, &m_sConfigEncoder);
    // const uint8_t u8Datas[] = { 0xCC, 0x01, 0x00, 0x04, 0xBA, 0xDC, 0x0F, 0xFE, 0x57, 0x99 };
    // UARTPROTOCOLDEC_HandleIn(&m_sHandleDecoder, u8Datas, sizeof(u8Datas));
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

    ManageServerConnection();

    // State machine ...
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
    STOVEMB_Take(portMAX_DELAY);
    STOVEMB_SMemBlock* pMemBlock = STOVEMB_GetMemBlock();

    // Any communication count
    m_sStateMachine.ttLastCommTicks = xTaskGetTickCount();

    ESP_LOGI(TAG, "Accepted frame, ID: %d, len: %d", u8ID, u16PayloadLen);
    // esp_event_post_to(EVENT_g_LoopHandle, MAINAPP_EVENT, REQUESTCONFIGRELOAD_EVENT, NULL, 0, 0);
    switch ((UFEC23PROTOCOL_FRAMEID)u8ID)
    {
        case UFEC23PROTOCOL_FRAMEID_S2CReqVersionResp:
        {
            UFEC23ENDEC_S2CReqVersionResp sS2CReqVersionResp;
            if (!UFEC23ENDEC_S2CReqVersionRespDecode(&sS2CReqVersionResp, u8Payloads, u16PayloadLen))
            {
                ESP_LOGE(TAG, "Error frame S2CReqVersionResp");
                break;
            }
            pMemBlock->sS2CReqVersionResp = sS2CReqVersionResp;
            pMemBlock->sS2CReqVersionRespIsSet = true;
            break;
        }
        case UFEC23PROTOCOL_FRAMEID_S2CReqPingAliveResp:
        {
            ESP_LOGI(TAG, "Received frame S2CReqPingAliveResp");
            break;
        }
        case UFEC23PROTOCOL_FRAMEID_S2CGetRunningSettingResp:
        {
            UFEC23ENDEC_S2CGetRunningSettingResp s2CGetRunningSettingResp;

            // Receive settings ...
            if (!UFEC23ENDEC_S2CGetRunningSettingRespDecode(&s2CGetRunningSettingResp, u8Payloads, u16PayloadLen))
            {
                ESP_LOGE(TAG, "Error frame S2CGetRunningSettingResp");
                break;
            }
            pMemBlock->s2CGetRunningSetting = s2CGetRunningSettingResp;
            pMemBlock->s2CGetRunningSettingIsSet = true;
            ESP_LOGI(TAG, "Received frame S2CGetRunningSettingResp");
            break;
        }

        case UFEC23PROTOCOL_FRAMEID_S2CGetParameterResp:
        {
            // Received (Get Parameter Resp)
            UFEC23ENDEC_S2CReqParameterGetResp s;
            if (!UFEC23ENDEC_S2CReqParameterGetRespDecode(&s, u8Payloads, u16PayloadLen))
            {
                ProcParameterAbortDownload();
                break;
            }

            bool isOverflow = false;

            if (s.bHasRecord)
            {
                // Overflow detected ...
                if (pMemBlock->u32ParameterCount + 1 > STOVEMB_MAXIMUMSETTING_ENTRIES)
                {
                    ESP_LOGW(TAG, "Overflow, too many parameter received from the device");
                    isOverflow = true;
                }
                else
                {
                    memcpy(&pMemBlock->arrParameterEntries[pMemBlock->u32ParameterCount], &s.sEntry, sizeof(UFEC23ENDEC_SEntry));
                    pMemBlock->u32ParameterCount++;
                }
            }

            // Request the next one ... until we get EOF flag
            if (s.bIsEOF || isOverflow)
            {
                m_sStateMachine.eProcParameterEntry = EPROCPARAMETERENTRY_None;
                pMemBlock->bIsParameterDownloadCompleted = true;
                ESP_LOGI(TAG, "Parameter download done, entries: %d", pMemBlock->u32ParameterCount);
                break;
            }

            UFEC23ENDEC_C2SReqParameterGet sC2SReqParameterGet = 
            {
                .eIterateOp = UFEC23ENDEC_EITERATEOP_Next
            };
            const int32_t n = UFEC23ENDEC_C2SReqParameterGetEncode(m_u8UARTSendProtocols, SENDPROTOCOL_COUNT, &sC2SReqParameterGet);
            UARTBRIDGE_SendFrame(UFEC23PROTOCOL_FRAMEID_C2SGetParameter, m_u8UARTSendProtocols, n);
            break;
        }
        default:
            break;
    }

    STOVEMB_Give();
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

static void ManageServerConnection()
{
    // Business logics
    const TickType_t ttDiffLastComm = (xTaskGetTickCount() - m_sStateMachine.ttLastCommTicks);
    if (ttDiffLastComm > pdMS_TO_TICKS(UARTBRIDGE_COMMUNICATIONLOST_TIMEOUT_MS))
    {
        if (m_sStateMachine.bIsConnected)
        {
            m_sStateMachine.bIsConnected = false;
            ServerDisconnected();
        }
    }
    else
    {
        if (!m_sStateMachine.bIsConnected)
        {
            m_sStateMachine.bIsConnected = true;
            ServerConnected();
        }
    }
    
    // Send keep alive if no communication happened for some time ...
    if (ttDiffLastComm > pdMS_TO_TICKS(UARTBRIDGE_KEEPALIVE_MS))
    {
        UARTBRIDGE_SendFrame(UFEC23PROTOCOL_FRAMEID_C2SReqPingAlive, NULL, 0);
    }
}

static void ServerConnected()
{
    // Connected ...
    ESP_LOGI(TAG, "Server Connected");

    // Send some requests ...
    UARTBRIDGE_SendFrame(UFEC23PROTOCOL_FRAMEID_C2SReqVersion, NULL, 0);
    UARTBRIDGE_SendFrame(UFEC23PROTOCOL_FRAMEID_C2SGetRunningSetting, NULL, 0);

    // Start downloading parameters
    ProcParameterStartDownload();
}

static void ServerDisconnected()
{
    // Disconnected ...
    ESP_LOGI(TAG, "Server disconnected");
    ProcParameterAbortDownload();
}

static void ProcParameterStartDownload()
{
    STOVEMB_Take(portMAX_DELAY);
    STOVEMB_SMemBlock* pMB = STOVEMB_GetMemBlock();

    // Reset
    m_sStateMachine.eProcParameterEntry = EPROCPARAMETERENTRY_WaitParameterResponse;
    m_sStateMachine.ttParameterStartDownTicks = xTaskGetTickCount();

    pMB->u32ParameterCount = 0;
    pMB->bIsParameterDownloadCompleted = false;

    UFEC23ENDEC_C2SReqParameterGet sC2SReqParameterGet = 
    {
        .eIterateOp = UFEC23ENDEC_EITERATEOP_First
    };
    const int32_t n = UFEC23ENDEC_C2SReqParameterGetEncode(m_u8UARTSendProtocols, SENDPROTOCOL_COUNT, &sC2SReqParameterGet);
    UARTBRIDGE_SendFrame(UFEC23PROTOCOL_FRAMEID_C2SGetParameter, m_u8UARTSendProtocols, n);
    STOVEMB_Give();
}

static void ProcParameterAbortDownload()
{
    if (m_sStateMachine.eProcParameterEntry != EPROCPARAMETERENTRY_None)
    {
        ESP_LOGE(TAG, "Unable to decode get parameter");
        return;
    }

    STOVEMB_Take(portMAX_DELAY);
    STOVEMB_SMemBlock* pMB = STOVEMB_GetMemBlock();
    m_sStateMachine.eProcParameterEntry = EPROCPARAMETERENTRY_None;
    pMB->bIsParameterDownloadCompleted = false;
    pMB->u32ParameterCount = 0;
    STOVEMB_Give();
}