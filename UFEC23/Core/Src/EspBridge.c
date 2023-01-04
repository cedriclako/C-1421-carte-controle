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

#define MAX_RX_SIZE 1024

static void EncWriteUART(const UARTPROTOCOLENC_SHandle* psHandle, const uint8_t u8Datas[], uint32_t u32DataLen);

// Callbacks
static void DecAcceptFrame(const UARTPROTOCOLDEC_SHandle* psHandle, uint8_t u8ID, const uint8_t u8Payloads[], uint16_t u16PayloadLen);
static void DecDropFrame(const UARTPROTOCOLDEC_SHandle* psHandle, const char* szReason);
static int64_t GetTimerCountMS(const UARTPROTOCOLDEC_SHandle* psHandle);

osSemaphoreId ESP_UART_SemaphoreHandle;
static UARTPROTOCOLENC_SConfig m_sConfigEncoder =
{
    // Callbacks
    .fnWriteCb = EncWriteUART
};

static uint8_t m_u8UARTProtocolBuffers[MAX_RX_SIZE];

static UARTPROTOCOLDEC_SConfig m_sConfigDecoder =
{
    .u8PayloadBuffers = m_u8UARTProtocolBuffers,
    .u16PayloadBufferLen = sizeof(m_u8UARTProtocolBuffers),

    .u32FrameReceiveTimeOutMS = 50,

    // Callbacks
    .fnAcceptFrameCb = DecAcceptFrame,
    .fnDropFrameCb = DecDropFrame,
    // .fnGetTimerCountMSCb = GetTimerCountMS
};

static UARTPROTOCOLENC_SHandle m_sHandleEncoder;
static UARTPROTOCOLDEC_SHandle m_sHandleDecoder;
static uint16_t last_DMA_count;

void ESPMANAGER_Init()
{
	last_DMA_count = 0;
    // Encoder
    UARTPROTOCOLENC_Init(&m_sHandleEncoder, &m_sConfigEncoder);

    // Decoder
    UARTPROTOCOLDEC_Init(&m_sHandleDecoder, &m_sConfigDecoder);
}

void EspManager(void const * argument) {

	osSemaphoreDef(ESP_UART_SemaphoreHandle);
	ESP_UART_SemaphoreHandle = osSemaphoreCreate(osSemaphore(ESP_UART_SemaphoreHandle), 1);
	osSemaphoreWait(ESP_UART_SemaphoreHandle,1); //decrement semaphore value for the lack of way to create a semaphore with a count of 0.
	uint16_t DMA_count = 0;
	HAL_UARTEx_ReceiveToIdle_DMA(&huart2, m_u8UARTProtocolBuffers, MAX_RX_SIZE);

	for(;;) {

		DMA_count = (uint16_t)(MAX_RX_SIZE - hdma_usart2_rx.Instance->CNDTR);
		osDelay(100);
		const uint8_t u8DummyPayloads[] = { 'c', 'o', 'u', 'c', 'o', 'u' };
		UARTPROTOCOLENC_Send(&m_sHandleEncoder, 66, u8DummyPayloads, sizeof(u8DummyPayloads));
		UARTPROTOCOLENC_Send(&m_sHandleEncoder, 67, NULL, 0);

		if(DMA_count > last_DMA_count)
		{

			UARTPROTOCOLDEC_HandleIn(&m_sHandleDecoder,&m_u8UARTProtocolBuffers[last_DMA_count],(uint32_t)(DMA_count-last_DMA_count));
			last_DMA_count = DMA_count;
		}else if(DMA_count < last_DMA_count)
		{
			UARTPROTOCOLDEC_HandleIn(&m_sHandleDecoder,&m_u8UARTProtocolBuffers[last_DMA_count],(uint32_t)(MAX_RX_SIZE-last_DMA_count));

			UARTPROTOCOLDEC_HandleIn(&m_sHandleDecoder,m_u8UARTProtocolBuffers,(uint32_t)(DMA_count));
			last_DMA_count = DMA_count;
		}


	}

}

static void EncWriteUART(const UARTPROTOCOLENC_SHandle* psHandle, const uint8_t u8Datas[], uint32_t u32DataLen)
{
    //uart_write_bytes(HWGPIO_BRIDGEUART_PORT_NUM, u8Datas, u32DataLen);
	// Write byte into UART ...
	HAL_UART_Transmit(&huart2, u8Datas, u32DataLen, 500);
}


static void DecAcceptFrame(const UARTPROTOCOLDEC_SHandle* psHandle, uint8_t u8ID, const uint8_t u8Payloads[], uint16_t u16PayloadLen)
{
	//if(UFEC23ENDEC_C2SSetParameterDecode())
	//{

	//}

}

static void DecDropFrame(const UARTPROTOCOLDEC_SHandle* psHandle, const char* szReason)
{
    // Exists mostly for debug purpose
    // ESP_LOGE(TAG, "Dropped frame: %s", szReason);
}
