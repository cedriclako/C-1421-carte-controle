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
#include "uart_protocol_enc.h"
#include "uart_protocol_dec.h"
#include "ufec23_endec.h"
#include "ufec23_protocol.h"

//osSemaphoreId ESP_UART_SemaphoreHandle;
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

static uint8_t m_u8UARTProtocolBuffers[1024*8];

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

void ESPMANAGER_Init()
{
    // Encoder
    UARTPROTOCOLENC_Init(&m_sHandleEncoder, &m_sConfigEncoder);

    // Decoder
    UARTPROTOCOLDEC_Init(&m_sHandleDecoder, &m_sConfigDecoder);
}

void EspManager(void const * argument) {

	osSemaphoreDef(ESP_UART_SemaphoreHandle);
	ESP_UART_SemaphoreHandle = osSemaphoreCreate(osSemaphore(ESP_UART_SemaphoreHandle), 1);
	osSemaphoreWait(ESP_UART_SemaphoreHandle,1); //decrement semaphore value for the lack of way to create a semaphore with a count of 0.

	// TODO: Use: UARTPROTOCOLDEC_HandleIn()
	static uint8_t TX_BUFFER[2];

	TX_BUFFER[0] = 0x61;
	TX_BUFFER[1] = 0x62;


	for(;;) {

		osDelay(1000);


		HAL_UART_Transmit_IT(&huart2, TX_BUFFER, 2);

		if(osErrorOS == osSemaphoreWait(ESP_UART_SemaphoreHandle,500)) //wait 500ms for an answer or retry
		{
			//clearly something is wrong Abort the transmission
			//HAL_GPIO_WritePin(STATUS_LED0_GPIO_Port,STATUS_LED0_Pin,RESET);
			HAL_UART_Abort_IT(&huart2);
			HAL_UART_DeInit(&huart2);
			osDelay(100);
			MX_USART2_UART_Init();
			osDelay(100);
		}

	}

}

static void EncWriteUART(const UARTPROTOCOLENC_SHandle* psHandle, const uint8_t u8Datas[], uint32_t u32DataLen)
{
    //uart_write_bytes(HWGPIO_BRIDGEUART_PORT_NUM, u8Datas, u32DataLen);
	// Write byte into UART ...
}


static void DecAcceptFrame(const UARTPROTOCOLDEC_SHandle* psHandle, uint8_t u8ID, const uint8_t u8Payloads[], uint16_t u16PayloadLen)
{

}

static void DecDropFrame(const UARTPROTOCOLDEC_SHandle* psHandle, const char* szReason)
{
    // Exists mostly for debug purpose
    // ESP_LOGE(TAG, "Dropped frame: %s", szReason);
}
