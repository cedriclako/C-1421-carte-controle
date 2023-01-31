/*
Solutions Novika.
Copyright 2021
===========================================================================

Filename :      EspManager.c

Author(s):      Charles Richard, CPI # 6045522

Public prefix : EspManager

Project # : C-1421

Product: UFEC23

Creation date:  2022/11/21

Description:    Communication channel with ESP32

===========================================================================

Modifications
-------------

By    |   Date     | Version | Description
------+------------+---------+---------------------------------------------
CR    | 2022/11/21 | -       | Creation
===========================================================================
*/

#include "main.h"
#include "cmsis_os.h"
#include "stm32f1xx_hal.h"
#include "EspBridge.h"
#include "uart_protocol/uart_protocol_enc.h"
#include "uart_protocol/uart_protocol_dec.h"
#include "ufec23_protocol/ufec23_endec.h"
#include "ufec23_protocol/ufec23_protocol.h"
#include "ParamFile.h"

typedef struct
{
	// GetParameter iterator data
	uint32_t u32GetParameterCurrentIndex;
} SBridgeState;

static void EncWriteUART(const UARTPROTOCOLENC_SHandle* psHandle, const uint8_t u8Datas[], uint32_t u32DataLen);

// =======
// UART protocol encoder buffers
#define MAX_RX_DMA_SIZE 1024
static uint8_t m_u8UART_RX_DMABuffers[MAX_RX_DMA_SIZE];
static uint16_t m_last_DMA_count;

// =======
// UART protocol encoder buffers
#define UART_OUTBUFFER_LEN (128)
static uint8_t m_u8UARTOutputBuffers[UART_OUTBUFFER_LEN];

static UARTPROTOCOLENC_SHandle m_sHandleEncoder;
static UARTPROTOCOLENC_SConfig m_sConfigEncoder =
{
    // Callbacks
    .fnWriteCb = EncWriteUART
};

// =======
// UART protocol decoder buffers
static uint8_t m_u8UARTProtocolBuffers2[UARTPROTOCOLCOMMON_MAXPAYLOAD];

static void DecAcceptFrame(const UARTPROTOCOLDEC_SHandle* psHandle, uint8_t u8ID, const uint8_t u8Payloads[], uint16_t u16PayloadLen);
static void DecDropFrame(const UARTPROTOCOLDEC_SHandle* psHandle, const char* szReason);
static int64_t GetTimerCountMS(const UARTPROTOCOLDEC_SHandle* psHandle);

static void UARTErrorCb(UART_HandleTypeDef *huart);

static UARTPROTOCOLDEC_SConfig m_sConfigDecoder =
{
    .u8PayloadBuffers = m_u8UARTProtocolBuffers2,
    .u16PayloadBufferLen = sizeof(m_u8UARTProtocolBuffers2),

    .u32FrameReceiveTimeOutMS = 50,

    // Callbacks
    .fnAcceptFrameCb = DecAcceptFrame,
    .fnDropFrameCb = DecDropFrame,
    .fnGetTimerCountMSCb = GetTimerCountMS
};

static UARTPROTOCOLDEC_SHandle m_sHandleDecoder;
static volatile bool m_bNeedRestartDMA = false;

// --------
// Bridge state
osSemaphoreId ESP_UART_SemaphoreHandle;
static SBridgeState m_sBridgeState;

void ESPMANAGER_Init()
{
	// Initialize bridge ...
	m_sBridgeState.u32GetParameterCurrentIndex = 0;

	m_last_DMA_count = 0;

    // Encoder
    UARTPROTOCOLENC_Init(&m_sHandleEncoder, &m_sConfigEncoder);

    // Decoder
    UARTPROTOCOLDEC_Init(&m_sHandleDecoder, &m_sConfigDecoder);
}

void ESPMANAGER_Task(void const * argument)
{
	osSemaphoreDef(ESP_UART_SemaphoreHandle);
	ESP_UART_SemaphoreHandle = osSemaphoreCreate(osSemaphore(ESP_UART_SemaphoreHandle), 1);
	osSemaphoreWait(ESP_UART_SemaphoreHandle,1); //decrement semaphore value for the lack of way to create a semaphore with a count of 0.

	HAL_UARTEx_ReceiveToIdle_DMA(&huart2, m_u8UART_RX_DMABuffers, MAX_RX_DMA_SIZE);
	HAL_UART_RegisterCallback(&huart2, HAL_UART_ERROR_CB_ID, UARTErrorCb);

	for(;;)
	{
		if (m_bNeedRestartDMA)
		{
			m_bNeedRestartDMA = false;
			HAL_UARTEx_ReceiveToIdle_DMA(&huart2, m_u8UART_RX_DMABuffers, MAX_RX_DMA_SIZE);
		}

		const uint16_t u16DMA_count = (uint16_t)(MAX_RX_DMA_SIZE - hdma_usart2_rx.Instance->CNDTR);

		if(u16DMA_count > m_last_DMA_count)
		{
			UARTPROTOCOLDEC_HandleIn(&m_sHandleDecoder,&m_u8UART_RX_DMABuffers[m_last_DMA_count],(uint16_t)(u16DMA_count-m_last_DMA_count));
			m_last_DMA_count = u16DMA_count;
		}
		else if(u16DMA_count < m_last_DMA_count)
		{
			UARTPROTOCOLDEC_HandleIn(&m_sHandleDecoder,&m_u8UART_RX_DMABuffers[m_last_DMA_count],(uint16_t)(MAX_RX_DMA_SIZE-m_last_DMA_count));

			if(u16DMA_count != 0)
				UARTPROTOCOLDEC_HandleIn(&m_sHandleDecoder,m_u8UART_RX_DMABuffers,(uint32_t)(u16DMA_count));

			m_last_DMA_count = u16DMA_count;
		}

		osDelay(50);
	}

}

static void UARTErrorCb(UART_HandleTypeDef *huart)
{
	// If there is not enough activity it seems to trigger an error
	// in that case we need to restart the DMA
	m_bNeedRestartDMA = true;
}

static void EncWriteUART(const UARTPROTOCOLENC_SHandle* psHandle, const uint8_t u8Datas[], uint32_t u32DataLen)
{
    //uart_write_bytes(HWGPIO_BRIDGEUART_PORT_NUM, u8Datas, u32DataLen);
	// Write byte into UART ...
	HAL_UART_Transmit(&huart2, (uint8_t*)u8Datas, (uint16_t)u32DataLen, 500);
}


static void DecAcceptFrame(const UARTPROTOCOLDEC_SHandle* psHandle, uint8_t u8ID, const uint8_t u8Payloads[], uint16_t u16PayloadLen)
{
	switch((UFEC23PROTOCOL_FRAMEID)u8ID)
	{
		case UFEC23PROTOCOL_FRAMEID_A2AReqPingAlive:
		{
			// Decode PING then send a response ...
			UFEC23ENDEC_A2AReqPingAlive reqPing;
			if (!UFEC23ENDEC_A2AReqPingAliveDecode(&reqPing, u8Payloads, u16PayloadLen))
			{
				// TODO: Throw something into logging UART?
				break;
			}

			const uint16_t u16Len = (uint16_t)UFEC23ENDEC_A2AReqPingAliveEncode(m_u8UARTOutputBuffers, UART_OUTBUFFER_LEN, &reqPing);
			UARTPROTOCOLENC_Send(&m_sHandleEncoder, UFEC23PROTOCOL_FRAMEID_A2AReqPingAliveResp, m_u8UARTOutputBuffers, u16Len);
			break;
		}
		//case UFEC23PROTOCOL_FRAMEID_C2SReqVersion:
		//	break;
		//case UFEC23PROTOCOL_FRAMEID_C2SGetRunningSetting:
		//	break;
		//case UFEC23PROTOCOL_FRAMEID_C2SSetRunningSetting:
		//	break;
		case UFEC23PROTOCOL_FRAMEID_C2SGetParameter:
		{
			UFEC23ENDEC_S2CReqParameterGetResp sResp;

			UFEC23ENDEC_C2SGetParameter param;
			if(!UFEC23ENDEC_C2SGetParameterDecode(&param, u8Payloads,(uint32_t) u16PayloadLen))
			{
				// TODO: Throw something into logging UART?
				break;
			}

			const uint32_t u32ParamEntryCount = PARAMFILE_GetParamEntryCount();

			// Restart the iterator
			if (param.eIterateOp == UFEC23ENDEC_EITERATEOP_First)
			{
				m_sBridgeState.u32GetParameterCurrentIndex = 0;
			}
			else if (param.eIterateOp == UFEC23ENDEC_EITERATEOP_Next)
			{
				m_sBridgeState.u32GetParameterCurrentIndex++; // Next record ....
			}

			sResp.bHasRecord = false;
			sResp.bIsEOF = true;

			// Until EOF ...
			if (m_sBridgeState.u32GetParameterCurrentIndex < u32ParamEntryCount)
			{
				// Get record
				const PFL_SParameterItem* pParamItem = PARAMFILE_GetParamEntryByIndex(m_sBridgeState.u32GetParameterCurrentIndex);
				if (pParamItem != NULL && pParamItem->eType == PFL_TYPE_Int32)
				{
					sResp.bHasRecord = true;
					sResp.bIsEOF = false;

					sResp.sEntry.eParamType = UFEC23ENDEC_EPARAMTYPE_Int32;
					sResp.sEntry.uType.sInt32.s32Default = pParamItem->uType.sInt32.s32Default;
					sResp.sEntry.uType.sInt32.s32Min = pParamItem->uType.sInt32.s32Min;
					sResp.sEntry.uType.sInt32.s32Max = pParamItem->uType.sInt32.s32Max;
					strcpy(sResp.sEntry.szKey, pParamItem->szKey);
					int32_t s32Value;
					PFL_GetValueInt32(&PARAMFILE_g_sHandle, pParamItem->szKey, &s32Value);
					sResp.uValue.s32Value = s32Value;
				}
			}

			const uint16_t u16Len = (uint16_t)UFEC23ENDEC_S2CGetParameterRespEncode(m_u8UARTOutputBuffers, UART_OUTBUFFER_LEN, &sResp);
			UARTPROTOCOLENC_Send(&m_sHandleEncoder, UFEC23PROTOCOL_FRAMEID_S2CGetParameterResp, m_u8UARTOutputBuffers, u16Len);
			break;
		}
		case UFEC23PROTOCOL_FRAMEID_C2SSetParameter:
		{
			UFEC23PROTOCOL_C2SSetParameter param;
			if(!UFEC23ENDEC_C2SSetParameterDecode(&param, u8Payloads,(uint32_t) u16PayloadLen))
			{
				// TODO: Throw something into logging UART?
				break;
			}

			const PFL_ESETRET setRet = PFL_SetValueInt32(&PARAMFILE_g_sHandle, param.szKey, param.uValue.s32Value);
			UFEC23PROTOCOL_S2CSetParameterResp s2cSetParameterResp =
			{
				// We don't give full precision ... ok or fail is enough for now.
				.eResult = (setRet == PFL_ESETRET_OK) ? UFEC23PROTOCOL_ERESULT_Ok : UFEC23PROTOCOL_ERESULT_Fail
			};
			const uint16_t u16Len = (uint16_t)UFEC23ENDEC_S2CSetParameterRespEncode(m_u8UARTOutputBuffers, UART_OUTBUFFER_LEN, &s2cSetParameterResp);
			UARTPROTOCOLENC_Send(&m_sHandleEncoder, UFEC23PROTOCOL_FRAMEID_S2CSetParameterResp, m_u8UARTOutputBuffers, u16Len);
			break;
		}
		//case UFEC23PROTOCOL_FRAMEID_C2SCommitParameter:
		//	break;
		default:
			// TODO: Not a valid protocol ID... Do something? Throw into UART log?
			break;
	}

}

static void DecDropFrame(const UARTPROTOCOLDEC_SHandle* psHandle, const char* szReason)
{
    // Exists mostly for debug purpose
}

static int64_t GetTimerCountMS(const UARTPROTOCOLDEC_SHandle* psHandle)
{
	return xTaskGetTickCount() * portTICK_PERIOD_MS;
}
