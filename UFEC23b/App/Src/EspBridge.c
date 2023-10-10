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
#include <inttypes.h>
#include "main.h"
#include "Algo.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "stm32f1xx_hal.h"
#include "EspBridge.h"
#include "Algo.h"
#include "TemperatureManager.h"
#include "../esp32/uart_protocol/uart_protocol_enc.h"
#include "../esp32/uart_protocol/uart_protocol_dec.h"
#include "../esp32/ufec23_protocol/ufec23_endec.h"
#include "../esp32/ufec23_protocol/ufec23_protocol.h"
#include "ParamFile.h"

typedef struct
{
	// GetParameter iterator data
	uint32_t u32GetParameterCurrentIndex;

	bool bIsBridgeReady;
} SBridgeState;

static void EncWriteUART(const UARTPROTOCOLENC_SHandle* psHandle, const uint8_t u8Datas[], uint32_t u32DataLen);
extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;
// =======
// UART protocol encoder buffers
#define MAX_RX_DMA_SIZE 1024
static uint8_t m_u8UART_RX_DMABuffers[MAX_RX_DMA_SIZE];
static uint16_t m_last_DMA_count;

// =======
// UART protocol encoder buffers
#define UART_OUTBUFFER_LEN (1024)
static uint8_t m_u8UARTOutputBuffers[UART_OUTBUFFER_LEN];

static UARTPROTOCOLENC_SHandle m_sHandleEncoder;
static UARTPROTOCOLENC_SConfig m_sConfigEncoder =
{
    // Callbacks
    .fnWriteCb = EncWriteUART
};

// =======
// UART protocol decoder buffers
static uint8_t m_u8UARTPayloadBuffers[UARTPROTOCOLCOMMON_MAXPAYLOAD];

static char m_szDebugStringJSON[990+1];

static void DecAcceptFrame(const UARTPROTOCOLDEC_SHandle* psHandle, uint8_t u8ID, const uint8_t u8Payloads[], uint32_t u32PayloadLen);
static void DecDropFrame(const UARTPROTOCOLDEC_SHandle* psHandle, const char* szReason);
static int64_t GetTimerCountMS(const UARTPROTOCOLDEC_SHandle* psHandle);

static void UARTErrorCb(UART_HandleTypeDef *huart);

static void SendEvent(UFEC23PROTOCOL_EVENTID eEventID);
static void SendDebugData();

static UARTPROTOCOLDEC_SConfig m_sConfigDecoder =
{
    .u8PayloadBuffers = m_u8UARTPayloadBuffers,
    .u32PayloadBufferLen = sizeof(m_u8UARTPayloadBuffers),

    .u32FrameReceiveTimeOutMS = 50,

    // Callbacks
    .fnAcceptFrameCb = DecAcceptFrame,
    .fnDropFrameCb = DecDropFrame,
    .fnGetTimerCountMSCb = GetTimerCountMS
};

static UARTPROTOCOLDEC_SHandle m_sHandleDecoder;
static volatile bool m_bNeedRestartDMA = false;

extern RTC_HandleTypeDef hrtc;

// --------
// Bridge state
static SBridgeState m_sBridgeState;

void ESPMANAGER_Init()
{
	// Initialize bridge ...
	m_sBridgeState.u32GetParameterCurrentIndex = 0;

	m_sBridgeState.bIsBridgeReady = false;

	m_last_DMA_count = 0;

    // Encoder
    UARTPROTOCOLENC_Init(&m_sHandleEncoder, &m_sConfigEncoder);

    // Decoder
    UARTPROTOCOLDEC_Init(&m_sHandleDecoder, &m_sConfigDecoder);

	HAL_UARTEx_ReceiveToIdle_DMA(&huart1, m_u8UART_RX_DMABuffers, MAX_RX_DMA_SIZE);
	HAL_UART_RegisterCallback(&huart1, HAL_UART_ERROR_CB_ID, UARTErrorCb);
}

void ESPMANAGER_SetReady(void)
{
	m_sBridgeState.bIsBridgeReady = true;
}

void ESPMANAGER_Run(void)
{
	static bool bIsUp = false;
	// A strange bug happens, if we send an event too fast during the initialization phase
	// we get an error on the other side.
	// will need to be investigated further.
	if (!bIsUp && xTaskGetTickCount() > pdMS_TO_TICKS(900))
	{
		SendEvent(UFEC23PROTOCOL_EVENTID_BootedUp);
		bIsUp = true;
	}

	if (m_bNeedRestartDMA)
	{
		m_bNeedRestartDMA = false;
		HAL_UARTEx_ReceiveToIdle_DMA(&huart1, m_u8UART_RX_DMABuffers, MAX_RX_DMA_SIZE);
	}

	const uint16_t u16DMA_count = (uint16_t)(MAX_RX_DMA_SIZE - hdma_usart1_rx.Instance->CNDTR);

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

	static TickType_t ttSendDebugData = 0;
	if ( (xTaskGetTickCount() - ttSendDebugData) > pdMS_TO_TICKS(5000) )
	{
		ttSendDebugData = xTaskGetTickCount();
		SendDebugData();
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
	HAL_UART_Transmit_IT(&huart1, (uint8_t*)u8Datas, (uint16_t)u32DataLen);
}


static void DecAcceptFrame(const UARTPROTOCOLDEC_SHandle* psHandle, uint8_t u8ID, const uint8_t u8Payloads[], uint32_t u32PayloadLen)
{
	// Do not accept any message until the process is fully initialized.
	if (!m_sBridgeState.bIsBridgeReady)
		return;

	const char* szErrorString = NULL;
	#define GOTO_ERROR(_szErrorString) \
		{ \
			szErrorString = _szErrorString; \
			goto ERROR; \
		} \

	switch((UFEC23PROTOCOL_FRAMEID)u8ID)
	{
		case UFEC23PROTOCOL_FRAMEID_A2AReqPingAlive:
		{
			// Decode PING then send a response ...
			UFEC23ENDEC_A2AReqPingAlive reqPing;
			if (!UFEC23ENDEC_A2AReqPingAliveDecode(&reqPing, u8Payloads, u32PayloadLen))
			{
				GOTO_ERROR("A2AReqPingAliveDecode");
			}

			const uint32_t u32Len = (uint32_t)UFEC23ENDEC_A2AReqPingAliveEncode(m_u8UARTOutputBuffers, UART_OUTBUFFER_LEN, &reqPing);
			UARTPROTOCOLENC_Send(&m_sHandleEncoder, UFEC23PROTOCOL_FRAMEID_A2AReqPingAliveResp, m_u8UARTOutputBuffers, u32Len);
			break;
		}
		//case UFEC23PROTOCOL_FRAMEID_C2SReqVersion:
		//	break;
		//case UFEC23PROTOCOL_FRAMEID_C2SGetRunningSetting:
		//	break;
		//case UFEC23PROTOCOL_FRAMEID_C2SSetRunningSetting:
		//	break;
		case UFEC23PROTOCOL_FRAMEID_C2SSendDebugData:
		{
			SendDebugData();
			break;
		}
		case UFEC23PROTOCOL_FRAMEID_C2SGetParameter:
		{
			UFEC23ENDEC_S2CReqParameterGetResp sResp;

			UFEC23ENDEC_C2SGetParameter param;
			if(!UFEC23ENDEC_C2SGetParameterDecode(&param, u8Payloads,(uint32_t) u32PayloadLen))
			{
				GOTO_ERROR("C2SGetParameterDecode");
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

			const uint32_t u32Len = (uint32_t)UFEC23ENDEC_S2CGetParameterRespEncode(m_u8UARTOutputBuffers, UART_OUTBUFFER_LEN, &sResp);
			UARTPROTOCOLENC_Send(&m_sHandleEncoder, UFEC23PROTOCOL_FRAMEID_S2CGetParameterResp, m_u8UARTOutputBuffers, u32Len);
			break;
		}
		case UFEC23PROTOCOL_FRAMEID_C2SSetParameter:
		{
			UFEC23PROTOCOL_C2SSetParameter param;
			if(!UFEC23ENDEC_C2SSetParameterDecode(&param, u8Payloads,(uint32_t) u32PayloadLen))
			{
				GOTO_ERROR("C2SSetParameterDecode");
			}

			const PFL_ESETRET setRet = PFL_SetValueInt32(&PARAMFILE_g_sHandle, param.szKey, param.uValue.s32Value);
			UFEC23PROTOCOL_S2CSetParameterResp s2cSetParameterResp =
			{
				// We don't give full precision ... ok or fail is enough for now.
				.eResult = (setRet == PFL_ESETRET_OK) ? UFEC23PROTOCOL_ERESULT_Ok : UFEC23PROTOCOL_ERESULT_Fail
			};
			const uint32_t u32Len = (uint32_t)UFEC23ENDEC_S2CSetParameterRespEncode(m_u8UARTOutputBuffers, UART_OUTBUFFER_LEN, &s2cSetParameterResp);
			UARTPROTOCOLENC_Send(&m_sHandleEncoder, UFEC23PROTOCOL_FRAMEID_S2CSetParameterResp, m_u8UARTOutputBuffers, u32Len);
			break;
		}
		//case UFEC23PROTOCOL_FRAMEID_C2SCommitParameter:
		//	break;
		default:
			// TODO: Not a valid protocol ID... Do something? Throw into UART log?
			break;
	}

	return;
	ERROR:
	printf("ESPBRIDGE: , error: %s", szErrorString);
}

static void DecDropFrame(const UARTPROTOCOLDEC_SHandle* psHandle, const char* szReason)
{
    // Exists mostly for debug purpose
}

static int64_t GetTimerCountMS(const UARTPROTOCOLDEC_SHandle* psHandle)
{
	return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

static void SendEvent(UFEC23PROTOCOL_EVENTID eEventID)
{
	const uint32_t u32Len = (uint32_t)UFEC23ENDEC_S2CEventEncode(m_u8UARTOutputBuffers, UART_OUTBUFFER_LEN, eEventID);
	UARTPROTOCOLENC_Send(&m_sHandleEncoder, UFEC23PROTOCOL_FRAMEID_S2CEvent, m_u8UARTOutputBuffers, u32Len);
}

static void SendDebugData()
{
	/* Technically I can use cJson instead, but it's lighter to just use a plain old string. */
	RTC_TimeTypeDef sTime;
	HAL_RTC_GetTime(&hrtc, &sTime,0);
	char rtcTimeString[12+1];
	sprintf(rtcTimeString, "%02i:%02i:%02i",sTime.Hours, sTime.Minutes, sTime.Seconds);

	const Mobj* pMobj = ALGO_GetObjData();

	int n = snprintf(m_szDebugStringJSON, sizeof(m_szDebugStringJSON)-1,
		"{ \"#\":\"%s\",\"Tbaffle\":%f,\"Tavant\":%f,\"Plenum\":%f,\"State\":\"%s\",\"tStat\":\"%s\",\"dTbaffle\":%f,\"FanSpeed\":%"PRId32",\"Grille\":%"PRId32",\"Prim\":%"PRId32",\"Sec\":%"PRId32",\"Tboard\":%.0f,\"Door\":\"%s\",\"PartCH0ON\":%"PRId32",\"PartCH1ON\":%"PRId32",\"PartCH0OFF\":%"PRId32",\"PartCH1OFF\":%"PRId32",\"PartVar\":%"PRId32",\"PartSlope\":%f,\"TPart\":%"PRId32",\"PartCurr\":%f,\"PartLuxON\":%"PRId32",\"PartLuxOFF\":%"PRId32",\"PartTime\":%"PRId32",\"dTavant\":%f }",
		/*#*/rtcTimeString,
		/*Tbaffle*/pMobj->fBaffleTemp,
		/*Tavant*/pMobj->fChamberTemp,
		/*Plenum*/pMobj->fPlenumTemp,
		/*State*/ALGO_GetStateString(ALGO_GetCurrentState()),
		/*tStat*/pMobj->bThermostatOn ? "ON" : "OFF",
		/*dTbaffle*/pMobj->fBaffleDeltaT,
		/*FanSpeed*/(int32_t)0,
		/*Grille*/(int32_t)(pMobj->sGrill.u8aperturePosSteps*9/10),
		/*Prim*/(int32_t)(pMobj->sPrimary.u8aperturePosSteps*9/10),
		/*Sec*/(int32_t)(pMobj->sSecondary.u8aperturePosSteps*9/10),
		/*Tboard*/get_BoardTemp(),
		/*Door*/pMobj->bDoorOpen ? "OPEN" : "CLOSED",
		/*PartCH0ON*/(int32_t)pMobj->sParticles->u16ch0_ON,
		/*PartCH1ON*/(int32_t)pMobj->sParticles->u16ch1_ON,
		/*PartCH0OFF*/(int32_t)pMobj->sParticles->u16ch0_OFF,
		/*PartCH1OFF*/(int32_t)pMobj->sParticles->u16ch1_OFF,
		/*PartVar*/(int32_t)pMobj->sParticles->u16stDev,
		/*PartSlope*/pMobj->sParticles->fslope,
		/*TPart*/(int32_t)pMobj->sParticles->u16temperature,
		/*PartCurr*/pMobj->sParticles->fLED_current_meas,
		/*PartLuxON*/(int32_t)pMobj->sParticles->u16Lux_ON,
		/*PartLuxOFF*/(int32_t)pMobj->sParticles->u16Lux_OFF,
		/*PartTime*/(int32_t)pMobj->sParticles->u16TimeSinceInit,
		/*dTavant*/pMobj->fChamberDeltaT
	);
	if (n <= 0)
	{
		strcpy(m_szDebugStringJSON, "{ \"error\" : \"buffer overflow\" }");
		return;
	}
	m_szDebugStringJSON[n] = '\0';

	const uint32_t u32Len = (uint32_t)UFEC23ENDEC_S2CSendDebugDataRespEncode(m_u8UARTOutputBuffers, UART_OUTBUFFER_LEN, m_szDebugStringJSON);
	UARTPROTOCOLENC_Send(&m_sHandleEncoder, UFEC23PROTOCOL_FRAMEID_S2CSendDebugDataResp, m_u8UARTOutputBuffers, u32Len);
}
