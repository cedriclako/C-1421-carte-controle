#include <stdint.h>
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
    EPARAMETERPROCESS_None,
    EPARAMETERPROCESS_Downloading,
    EPARAMETERPROCESS_Uploading,
} EPARAMETERPROCESS;

typedef struct 
{
    bool bIsConnected;

    // Connection 
    TickType_t ttLastCommTicks;
    TickType_t ttLastKeepAliveSent;

    // Parameter file download
    EPARAMETERPROCESS eProcParameterProcess;
    TickType_t ttParameterStartDownTicks;
    int32_t s32WriteLastIndex;
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
static void RequestConfigWriteEvent(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

static void ManageServerConnection();

// Events
static void ServerConnected();
static void ServerDisconnected();

static bool ProcParameterDownload();
static bool ProcParameterUpload();
static void ProcParameterAbort();

static bool SendSetParameter(const STOVEMB_SParameterEntry* psParamEntry);

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

    // Reset parameter process
    m_sStateMachine.eProcParameterProcess = EPARAMETERPROCESS_None;

    // Decoder
    UARTPROTOCOLDEC_Init(&m_sHandleDecoder, &m_sConfigDecoder);
    // Encoder
    UARTPROTOCOLENC_Init(&m_sHandleEncoder, &m_sConfigEncoder);
    // const uint8_t u8Datas[] = { 0xCC, 0x01, 0x00, 0x04, 0xBA, 0xDC, 0x0F, 0xFE, 0x57, 0x99 };
    // UARTPROTOCOLDEC_HandleIn(&m_sHandleDecoder, u8Datas, sizeof(u8Datas));
    // Register events
    esp_event_handler_register_with(EVENT_g_LoopHandle, MAINAPP_EVENT, REQUESTCONFIGRELOAD_EVENT, RequestConfigReloadEvent, NULL);
    esp_event_handler_register_with(EVENT_g_LoopHandle, MAINAPP_EVENT, REQUESTCONFIGWRITE_EVENT, RequestConfigWriteEvent, NULL);
}

void UARTBRIDGE_Handler()
{
    // Read data from the UART
    uint8_t u8UARTDriverBuffers[128];    
    uint8_t dbg[128];
    int len = 0;
    while ((len = uart_read_bytes(HWGPIO_BRIDGEUART_PORT_NUM, u8UARTDriverBuffers, sizeof(u8UARTDriverBuffers), 0)) > 0)
    {
        UARTPROTOCOLDEC_HandleIn(&m_sHandleDecoder, u8UARTDriverBuffers, len);
    }

    ManageServerConnection();

    // State machine ...
    // Expiration time ...
    if (m_sStateMachine.eProcParameterProcess != EPARAMETERPROCESS_None)
    {
        if ((xTaskGetTickCount() - m_sStateMachine.ttParameterStartDownTicks) > pdMS_TO_TICKS(UARTBRIDGE_PROCDOWNLOADUPLOAD_MS))
        {
            ESP_LOGE(TAG, "Process time expired");
            ProcParameterAbort();
        }
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
    STOVEMB_Take(portMAX_DELAY);
    STOVEMB_SMemBlock* pMemBlock = STOVEMB_GetMemBlock();

    // Any communication count
    if (m_sStateMachine.bIsConnected)
    {
        // If it's connected, we accept any message as stay alive
        m_sStateMachine.ttLastCommTicks = xTaskGetTickCount();
    }

    ESP_LOGI(TAG, "Accepted frame, ID: %d, len: %d", u8ID, u16PayloadLen);
    // esp_event_post_to(EVENT_g_LoopHandle, MAINAPP_EVENT, REQUESTCONFIGRELOAD_EVENT, NULL, 0, 0);
    switch ((UFEC23PROTOCOL_FRAMEID)u8ID)
    {
        // case UFEC23PROTOCOL_FRAMEID_S2CReqVersionResp:
        // {
        //     UFEC23ENDEC_S2CReqVersionResp sS2CReqVersionResp;
        //     if (!UFEC23ENDEC_S2CReqVersionRespDecode(&sS2CReqVersionResp, u8Payloads, u16PayloadLen))
        //     {
        //         ESP_LOGE(TAG, "Error frame S2CReqVersionResp");
        //         break;
        //     }
        //     pMemBlock->sS2CReqVersionResp = sS2CReqVersionResp;
        //     pMemBlock->sS2CReqVersionRespIsSet = true;
        //     break;
        // }
        case UFEC23PROTOCOL_FRAMEID_A2AReqPingAliveResp:
        {
            ESP_LOGI(TAG, "Received frame S2CReqPingAliveResp");
            // If it's connected, we accept any message as stay alive
            m_sStateMachine.ttLastCommTicks = xTaskGetTickCount();
            break;
        }
        // case UFEC23PROTOCOL_FRAMEID_S2CGetRunningSettingResp:
        // {
        //     UFEC23ENDEC_S2CGetRunningSettingResp s2CGetRunningSettingResp;

        //     // Receive settings ...
        //     if (!UFEC23ENDEC_S2CGetRunningSettingRespDecode(&s2CGetRunningSettingResp, u8Payloads, u16PayloadLen))
        //     {
        //         ESP_LOGE(TAG, "Error frame S2CGetRunningSettingResp");
        //         break;
        //     }
        //     pMemBlock->s2CGetRunningSetting = s2CGetRunningSettingResp;
        //     pMemBlock->s2CGetRunningSettingIsSet = true;
        //     ESP_LOGI(TAG, "Received frame S2CGetRunningSettingResp");
        //     break;
        // }

        case UFEC23PROTOCOL_FRAMEID_S2CGetParameterResp:
        {
            if (m_sStateMachine.eProcParameterProcess != EPARAMETERPROCESS_Downloading)
            {
                break;  // We don't care when we aren't downloading.
            }

            // Received (Get Parameter Resp)
            UFEC23ENDEC_S2CReqParameterGetResp s;
            if (!UFEC23ENDEC_S2CGetParameterRespDecode(&s, u8Payloads, u16PayloadLen))
            {
                ProcParameterAbort();
                break;
            }

            m_sStateMachine.ttParameterStartDownTicks = xTaskGetTickCount();

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
                    STOVEMB_SParameterEntry* psEntryChanged = &pMemBlock->arrParameterEntries[pMemBlock->u32ParameterCount];
                    memcpy(&psEntryChanged->sEntry, &s.sEntry, sizeof(UFEC23ENDEC_SEntry));
                    psEntryChanged->bIsNeedWrite = false;
                    psEntryChanged->sWriteValue = s.uValue;
                    pMemBlock->u32ParameterCount++;
                }
            }

            // Request the next one ... until we get EOF flag
            if (s.bIsEOF || isOverflow)
            {
                m_sStateMachine.eProcParameterProcess = EPARAMETERPROCESS_None;
                pMemBlock->bIsParameterDownloadCompleted = true;
                ESP_LOGI(TAG, "Parameter download done, entries: %d", pMemBlock->u32ParameterCount);
                break;
            }

            UFEC23ENDEC_C2SGetParameter sC2SReqParameterGet = 
            {
                .eIterateOp = UFEC23ENDEC_EITERATEOP_Next
            };
            const int32_t n = UFEC23ENDEC_C2SGetParameterEncode(m_u8UARTSendProtocols, SENDPROTOCOL_COUNT, &sC2SReqParameterGet);
            UARTBRIDGE_SendFrame(UFEC23PROTOCOL_FRAMEID_C2SGetParameter, m_u8UARTSendProtocols, n);
            break;
        }
        case UFEC23PROTOCOL_FRAMEID_S2CSetParameterResp:
        {
            if (m_sStateMachine.eProcParameterProcess != EPARAMETERPROCESS_Uploading)
            {
                break;  // We don't care when we aren't downloading.
            }
            // Received (Get Parameter Resp)
            UFEC23PROTOCOL_S2CSetParameterResp s;
            if (!UFEC23ENDEC_S2CSetParameterRespDecode(&s, u8Payloads, u16PayloadLen))
            {
                ProcParameterAbort();
                break;
            }

            // The stove can refuse to set parameter.
            pMemBlock->bIsAnyUploadError |= (s.eResult != UFEC23PROTOCOL_ERESULT_Ok);

            m_sStateMachine.ttParameterStartDownTicks = xTaskGetTickCount();
            
            STOVEMB_SParameterEntry sParamEntry;
            const int32_t s32Index = STOVEMB_FindNextWritable(m_sStateMachine.s32WriteLastIndex+1, &sParamEntry);

            // The last one has been uploaded ...
            if (s32Index < 0)
            {
                // Upload completed.
                STOVEMB_ResetAllParameterWriteFlag();

                m_sStateMachine.eProcParameterProcess = EPARAMETERPROCESS_None;
                ESP_LOGI(TAG, "Parameter upload process done");
            }
            else
            {
                m_sStateMachine.s32WriteLastIndex = s32Index;               
                SendSetParameter(&sParamEntry);
            }
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
    ProcParameterDownload();
}

static void RequestConfigWriteEvent(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "RequestConfigWriteEvent");
    ProcParameterUpload();
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
    else if (m_sStateMachine.ttLastCommTicks != 0)
    {
        if (!m_sStateMachine.bIsConnected)
        {
            m_sStateMachine.bIsConnected = true;
            ServerConnected();
        }
    }
    
    // Send keep alive if no communication happened for some time ...
    if (ttDiffLastComm > pdMS_TO_TICKS(UARTBRIDGE_KEEPALIVE_MS) &&
        (xTaskGetTickCount() - m_sStateMachine.ttLastKeepAliveSent) > pdMS_TO_TICKS(UARTBRIDGE_KEEPALIVE_MS))
    {
        //ESP_LOGI(TAG, "UFEC23PROTOCOL_FRAMEID_A2AReqPingAlive");
        UARTBRIDGE_SendFrame(UFEC23PROTOCOL_FRAMEID_A2AReqPingAlive, NULL, 0);
        m_sStateMachine.ttLastKeepAliveSent = xTaskGetTickCount(); 
    }
}

static void ServerConnected()
{
    // Connected ...
    ESP_LOGI(TAG, "Server Connected");

    STOVEMB_GetMemBlock()->bIsStoveConnectedAndReady = true;

    // Send some requests ...
    // UARTBRIDGE_SendFrame(UFEC23PROTOCOL_FRAMEID_C2SReqVersion, NULL, 0);
    //UARTBRIDGE_SendFrame(UFEC23PROTOCOL_FRAMEID_C2SGetRunningSetting, NULL, 0);
    
    // Start downloading parameters
    ProcParameterDownload();
}

static void ServerDisconnected()
{   
    STOVEMB_GetMemBlock()->bIsStoveConnectedAndReady = false;

    // Disconnected ...
    ESP_LOGI(TAG, "Server disconnected");
    ProcParameterAbort();
}

static bool SendSetParameter(const STOVEMB_SParameterEntry* psParamEntry)
{
    UFEC23PROTOCOL_C2SSetParameter sParam;
    strcpy(sParam.szKey, psParamEntry->sEntry.szKey);
    memcpy(&sParam.uValue, &psParamEntry->sWriteValue, sizeof(UFEC23ENDEC_uValue));
    const int32_t n = UFEC23ENDEC_C2SSetParameterEncode(m_u8UARTSendProtocols, SENDPROTOCOL_COUNT, &sParam);
    if (n == 0)
        return false;
        
    ESP_LOGI(TAG, "Parameter upload, key: %s", psParamEntry->sEntry.szKey);
    UARTBRIDGE_SendFrame(UFEC23PROTOCOL_FRAMEID_C2SSetParameter, m_u8UARTSendProtocols, n);
    return true;
}

static void ProcParameterAbort()
{
    if (m_sStateMachine.eProcParameterProcess == EPARAMETERPROCESS_Downloading)
    {
        ESP_LOGE(TAG, "Unable to download process aborted");
        STOVEMB_Take(portMAX_DELAY);
        STOVEMB_SMemBlock* pMB = STOVEMB_GetMemBlock();
        pMB->bIsParameterDownloadCompleted = false;
        pMB->u32ParameterCount = 0;
        STOVEMB_Give();
    }

    m_sStateMachine.eProcParameterProcess = EPARAMETERPROCESS_None;
}

static bool ProcParameterDownload()
{
    if (m_sStateMachine.eProcParameterProcess != EPARAMETERPROCESS_None)
    {
        ESP_LOGW(TAG, "Parameter download is already in progress");
        return false;
    }

    STOVEMB_Take(portMAX_DELAY);
    STOVEMB_SMemBlock* pMB = STOVEMB_GetMemBlock();

    // Reset
    m_sStateMachine.eProcParameterProcess = EPARAMETERPROCESS_Downloading;
    m_sStateMachine.ttParameterStartDownTicks = xTaskGetTickCount();

    pMB->u32ParameterCount = 0;
    pMB->bIsParameterDownloadCompleted = false;

    UFEC23ENDEC_C2SGetParameter sC2SReqParameterGet = 
    {
        .eIterateOp = UFEC23ENDEC_EITERATEOP_First
    };
    STOVEMB_Give();

    const int32_t n = UFEC23ENDEC_C2SGetParameterEncode(m_u8UARTSendProtocols, SENDPROTOCOL_COUNT, &sC2SReqParameterGet);
    UARTBRIDGE_SendFrame(UFEC23PROTOCOL_FRAMEID_C2SGetParameter, m_u8UARTSendProtocols, n);
    return true;
}

static bool ProcParameterUpload()
{
    if (m_sStateMachine.eProcParameterProcess != EPARAMETERPROCESS_None)
    {
        ESP_LOGE(TAG, "Unable to upload parameters");
        return false;
    }

    m_sStateMachine.s32WriteLastIndex = 0;
    
    STOVEMB_Take(portMAX_DELAY);
    // Upload
    STOVEMB_GetMemBlock()->bIsAnyUploadError = false;
    STOVEMB_SParameterEntry sParamEntry;
    const int32_t s32Index = STOVEMB_FindNextWritable(0, &sParamEntry);
    STOVEMB_Give();

    // Return true because it's a normal usecase to not have anything to upload
    if (s32Index < 0)
        return true;

    m_sStateMachine.eProcParameterProcess = EPARAMETERPROCESS_None;

    if (!SendSetParameter(&sParamEntry))
    {
        ESP_LOGE(TAG, "Unable to encode parameter write command");
        return false;
    }

    // Declare the process started
    m_sStateMachine.eProcParameterProcess = EPARAMETERPROCESS_Uploading;
    m_sStateMachine.ttParameterStartDownTicks = xTaskGetTickCount();
    return true;
}
