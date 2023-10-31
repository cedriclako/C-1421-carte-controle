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
#include "DebugPort.h"
#include "Algo.h"
#include "TemperatureManager.h"
#include "../esp32/uart_protocol/uart_protocol_enc.h"
#include "../esp32/uart_protocol/uart_protocol_dec.h"
#include "../esp32/ufec23_protocol/ufec23_endec.h"
#include "../esp32/ufec23_protocol/ufec23_protocol.h"
#include "../esp32/ufec23_protocol/ufec_stream.h"
#include "ParamFile.h"

#define TAG "ESPBridge"

typedef struct
{
	// GetParameter iterator data
	uint32_t u32GetParameterCurrentIndex;

	bool bIsBridgeReady;
} SBridgeState;

typedef struct _SScheduler SScheduler;

typedef void (*FPReadFrame)(const SScheduler* pSchContext);
typedef void (*FPWriteFrame)(const SScheduler* pSchContext, const uint8_t* pu8Data, uint32_t u32Len);

typedef struct _SScheduler
{
	const char* szName;
	UFEC23PROTOCOL_FRAMEID eFrameID;
	uint16_t u16DelayMS;
	FPReadFrame fpRead;
	FPWriteFrame fpWrite;

	// State
	TickType_t ttLastSend;
} SScheduler;

#define SSCHEDULER_INIT(_frameID, _delayMS, _fpRead, _fpWrite) { .szName = #_frameID, .eFrameID = _frameID, .u16DelayMS =_delayMS, .fpRead = _fpRead, .fpWrite = _fpWrite, .ttLastSend = 0 }

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
static void UARTTXCompleteCb(UART_HandleTypeDef *huart);

// Read / Write variable and events
static void SendEvent(UFEC23PROTOCOL_EVENTID eEventID);

static void SendDebugData(const SScheduler* pSchContext);
static void ReadVariableFrame(const SScheduler* pSchContext);
static void WriteVariableFrame(const SScheduler* pSchContext, const uint8_t* pu8Data, uint32_t u32Len);

static void HandleScheduler(bool bIsForceSending);

static SScheduler m_sSchedulers[] =
{
	// Frame												Delay (ms)      Read   				Write
	SSCHEDULER_INIT(UFEC23PROTOCOL_FRAMEID_StatRmt, 			 1000, 		ReadVariableFrame, 	WriteVariableFrame),
	SSCHEDULER_INIT(UFEC23PROTOCOL_FRAMEID_LowerSpeedRmt, 		 1000, 		ReadVariableFrame, 	WriteVariableFrame),
	SSCHEDULER_INIT(UFEC23PROTOCOL_FRAMEID_DistribSpeedRmt,  	 1000, 		ReadVariableFrame, 	WriteVariableFrame),
	SSCHEDULER_INIT(UFEC23PROTOCOL_FRAMEID_BoostStatRmt, 		 1000, 		ReadVariableFrame, 	WriteVariableFrame),
	SSCHEDULER_INIT(UFEC23PROTOCOL_FRAMEID_DebugDataString, 	 5000, 		SendDebugData, 		NULL)
};
#define SSCHEDULER_COUNT (sizeof(m_sSchedulers)/sizeof(m_sSchedulers[0]))

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
static volatile bool m_TransmitInProgress = false;

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
	HAL_UART_RegisterCallback(&huart1, HAL_UART_TX_COMPLETE_CB_ID, UARTTXCompleteCb);
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


	// Don't send anything until the bridge is ready.
	if (m_sBridgeState.bIsBridgeReady)
	{
		HandleScheduler(false);
	}
}

static void HandleScheduler(bool bIsForceSending)
{
	// Scheduler, period data sent
	for(int i = 0; i < SSCHEDULER_COUNT; i++)
	{
		SScheduler* pSch = &m_sSchedulers[i];

		if ( bIsForceSending || (xTaskGetTickCount() - pSch->ttLastSend) > pdMS_TO_TICKS(pSch->u16DelayMS) )
		{
			pSch->ttLastSend = xTaskGetTickCount();
			if (pSch->fpRead != NULL)
				pSch->fpRead(pSch);
		}
	}
}

static void UARTErrorCb(UART_HandleTypeDef *huart)
{
	// If there is not enough activity it seems to trigger an error
	// in that case we need to restart the DMA
	m_bNeedRestartDMA = true;
	m_TransmitInProgress = false;
}

static void UARTTXCompleteCb(UART_HandleTypeDef *huart)
{
	m_TransmitInProgress = false;
}

static void EncWriteUART(const UARTPROTOCOLENC_SHandle* psHandle, const uint8_t u8Datas[], uint32_t u32DataLen)
{
    //uart_write_bytes(HWGPIO_BRIDGEUART_PORT_NUM, u8Datas, u32DataLen);
	// Write byte into UART ...
	m_TransmitInProgress = true;
	HAL_UART_Transmit_IT(&huart1, (uint8_t*)u8Datas, (uint16_t)u32DataLen);
	while(m_TransmitInProgress);
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
		case UFEC23PROTOCOL_FRAMEID_C2SSyncAll:
		{
			LOG(TAG, "Server sync request received");
			// Force to send all variables
			HandleScheduler(true);
			// Confirm the dump all is complete
			UARTPROTOCOLENC_Send(&m_sHandleEncoder, UFEC23PROTOCOL_FRAMEID_S2CSyncAll, m_u8UARTOutputBuffers, 0);
			LOG(TAG, "Server sync request done");
			break;
		}
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
					sResp.sEntry.eEntryFlag = ((pParamItem->eOpt & PFL_EOPT_IsVolatile) == PFL_EOPT_IsVolatile) ?
							UFEC23ENDEC_EENTRYFLAGS_Volatile : UFEC23ENDEC_EENTRYFLAGS_None;
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
		case UFEC23PROTOCOL_FRAMEID_C2SCommitParameter:
			PFL_CommitAll(&PARAMFILE_g_sHandle);
			break;
		default:
			// Check if any values related to the scheduler can be written
			for(int i = 0; i < SSCHEDULER_COUNT; i++)
			{
				SScheduler* pSch = &m_sSchedulers[i];
				if (pSch->eFrameID == u8ID && pSch->fpWrite != NULL)
				{
					pSch->fpWrite(pSch, u8Payloads,(uint32_t) u32PayloadLen);
					break;
				}
			}
			// TODO: Not a valid protocol ID... Do something? Throw into UART log?
			GOTO_ERROR("Unhandled frameID");
			break;
	}

	return;
	ERROR:
	LOG(TAG, "ESPBRIDGE: error: %s", szErrorString);
}

static void DecDropFrame(const UARTPROTOCOLDEC_SHandle* psHandle, const char* szReason)
{
    // Exists mostly for debug purpose
	LOG(TAG, "Dropped frame: %s", szReason);
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

static void ReadVariableFrame(const SScheduler* pSchContext)
{
	int32_t s32Count = 0;
	int32_t s32Value = 0;

	switch(pSchContext->eFrameID)
	{
		case UFEC23PROTOCOL_FRAMEID_StatRmt:
		{
			if (PFL_GetValueInt32(&PARAMFILE_g_sHandle, PFD_RMT_TSTAT, &s32Value) == PFL_ESETRET_OK)
				s32Count = UFEC23ENDEC_S2CEncodeS32(m_u8UARTOutputBuffers, UART_OUTBUFFER_LEN, s32Value);
			break;
		}
		case UFEC23PROTOCOL_FRAMEID_LowerSpeedRmt:
		{
			if (PFL_GetValueInt32(&PARAMFILE_g_sHandle, PFD_RMT_LOWFAN, &s32Value) == PFL_ESETRET_OK)
				s32Count = UFEC23ENDEC_S2CEncodeS32(m_u8UARTOutputBuffers, UART_OUTBUFFER_LEN, s32Value);
			break;
		}
		case UFEC23PROTOCOL_FRAMEID_DistribSpeedRmt:
		{
			if (PFL_GetValueInt32(&PARAMFILE_g_sHandle, PFD_RMT_DISTFAN, &s32Value) == PFL_ESETRET_OK)
				s32Count = UFEC23ENDEC_S2CEncodeS32(m_u8UARTOutputBuffers, UART_OUTBUFFER_LEN, s32Value);
			break;
		}
		case UFEC23PROTOCOL_FRAMEID_BoostStatRmt:
		{
			if (PFL_GetValueInt32(&PARAMFILE_g_sHandle, PFD_RMT_BOOST, &s32Value) == PFL_ESETRET_OK)
				s32Count = UFEC23ENDEC_S2CEncodeS32(m_u8UARTOutputBuffers, UART_OUTBUFFER_LEN, s32Value);
			break;
		}
		default:
			LOG(TAG, "Read is not implemented for %s", pSchContext->szName);
			break;
	}

	if (s32Count > 0)
		UARTPROTOCOLENC_Send(&m_sHandleEncoder, UFEC23PROTOCOL_FRAMESVRRESP(pSchContext->eFrameID), m_u8UARTOutputBuffers, s32Count);
	else
		LOG(TAG, "ReadVariable seems not implemented for: %s", pSchContext->szName);
}

static void WriteVariableFrame(const SScheduler* pSchContext, const uint8_t* pu8Data, uint32_t u32Len)
{
	#define SetValue(_szName) do { \
		int32_t s32Value; \
		if (UFEC23ENDEC_S2CDecodeS32(&s32Value, pu8Data, u32Len)) { \
			PFL_ESETRET ret; \
			if ((ret = PFL_SetValueInt32(&PARAMFILE_g_sHandle, _szName, s32Value)) != PFL_ESETRET_OK) { \
				LOG(TAG, "Unable to set value: %s, result: %" PRId32, pSchContext->szName, (int32_t)ret); \
			} \
			else {\
				LOG(TAG, "Wiring value: %s, value: %" PRId32, pSchContext->szName, s32Value); \
				ReadVariableFrame(pSchContext); \
			} \
		} else { \
			LOG(TAG, "Failed decoding: %s", pSchContext->szName); \
		} \
	} while(0);

	switch(pSchContext->eFrameID)
	{
		case UFEC23PROTOCOL_FRAMEID_StatRmt:
			SetValue(PFD_RMT_TSTAT);
			break;
		case UFEC23PROTOCOL_FRAMEID_LowerSpeedRmt:
			SetValue(PFD_RMT_LOWFAN);
			break;
		case UFEC23PROTOCOL_FRAMEID_DistribSpeedRmt:
			SetValue(PFD_RMT_DISTFAN);
			break;
		case UFEC23PROTOCOL_FRAMEID_BoostStatRmt:
			SetValue(PFD_RMT_BOOST);
			break;
		default:
			LOG(TAG, "Write is not implemented for %s, count: %"PRIu32, pSchContext->szName, u32Len);
			break;
	}
}

static void SendDebugData(const SScheduler* pSchContext)
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
	UARTPROTOCOLENC_Send(&m_sHandleEncoder, UFEC23PROTOCOL_FRAMESVRRESP(pSchContext->eFrameID), m_u8UARTOutputBuffers, u32Len);
}
