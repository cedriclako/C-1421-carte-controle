#ifndef _STM32PROTOCOL_H_
#define _STM32PROTOCOL_H_

#include <stdint.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

typedef struct
{
	// Init GPIO
	bool bInitGPIO;			/* Mandatory*/
	gpio_num_t reset_pin;	/* Mandatory*/
	gpio_num_t boot0_pin;	/* Mandatory*/
	gpio_num_t boot1_pin;	/* Optional */
	// Init UART
	bool bInitUART;			/* Mandatory*/
	uart_port_t uart_port;	/* Mandatory*/
	gpio_num_t uart_tx_pin;	/* Optional (if bInitUART = FALSE)*/
	gpio_num_t uart_rx_pin;	/* Optional (if bInitUART = FALSE)*/

	// Config
	uint32_t u32RecvDataTimeoutMS;	/* Mandatory*/
	uint32_t u32SyncDataTimeoutMS;	/* Mandatory*/
} STM32PROTOCOL_SConfig;

typedef struct
{
	const STM32PROTOCOL_SConfig* psConfig;
} STM32PROTOCOL_SContext;

#define STM32PROTOCOL_SERIAL_BAUDRATE (115200)

#define STM32PROTOCOL_ACK (0x79)
#define STM32PROTOCOL_NACK (0x1F)

#define STM32PROTOCOL_SCONFIG_INIT { .bInitGPIO = false, .reset_pin = -1, .boot0_pin = -1, .boot1_pin = -1, .bInitUART = false, .uart_port = -1, .uart_tx_pin = -1, .uart_rx_pin = -1, .u32RecvDataTimeoutMS = 1000, .u32SyncDataTimeoutMS = 10000 }

void STM32PROTOCOL_Init(STM32PROTOCOL_SContext* pContext, const STM32PROTOCOL_SConfig* pConfig);

esp_err_t STM32PROTOCOL_ResetSTM(const STM32PROTOCOL_SContext* pContext, bool bIsBootloaderMode);

esp_err_t STM32PROTOCOL_SetupSTM(const STM32PROTOCOL_SContext* pContext);

int STM32PROTOCOL_EndConn(const STM32PROTOCOL_SContext* pContext);

esp_err_t STM32PROTOCOL_CmdSync(const STM32PROTOCOL_SContext* pContext);

esp_err_t STM32PROTOCOL_CmdVersion(const STM32PROTOCOL_SContext* pContext);

esp_err_t STM32PROTOCOL_CmdId(const STM32PROTOCOL_SContext* pContext);

esp_err_t STM32PROTOCOL_CmdGet(const STM32PROTOCOL_SContext* pContext);

esp_err_t STM32PROTOCOL_CmdErase(const STM32PROTOCOL_SContext* pContext);

esp_err_t STM32PROTOCOL_CmdExtErase(const STM32PROTOCOL_SContext* pContext);

esp_err_t STM32PROTOCOL_CmdWrite(const STM32PROTOCOL_SContext* pContext);

esp_err_t STM32PROTOCOL_CmdRead(const STM32PROTOCOL_SContext* pContext);

esp_err_t STM32PROTOCOL_IncrementLoadAddress(const STM32PROTOCOL_SContext* pContext, char *loadAddr);

int STM32PROTOCOL_LoadAddress(const STM32PROTOCOL_SContext* pContext, const char adrMS, const char adrMI, const char adrLI, const char adrLS);

esp_err_t STM32PROTOCOL_ReadPage(const STM32PROTOCOL_SContext* pContext, const char *address, const uint8_t* data);

esp_err_t STM32PROTOCOL_FlashPage(const STM32PROTOCOL_SContext* pContext, const char *address, const uint8_t* data);

#endif
