#ifndef _HARDWAREGPIO_H_
#define _HARDWAREGPIO_H_

#include <stdbool.h>

#include "driver/uart.h"
#include "driver/gpio.h"

#define HWGPIO_SANITY_LED_PIN (0)

#define HWGPIO_BRIDGEUART_TXD (17)
#define HWGPIO_BRIDGEUART_RXD (16)
#define HWGPIO_BRIDGEUART_RTS (UART_PIN_NO_CHANGE)
#define HWGPIO_BRIDGEUART_CTS (UART_PIN_NO_CHANGE)

#define HWGPIO_BRIDGEUART_PORT_NUM      (UART_NUM_2)
#define HWGPIO_BRIDGEUART_BAUDRATE      (115200)

#define HWGPIO_BRIDGEUART_BUFFSIZE 255

void HARDWAREGPIO_Init();

void HARDWAREGPIO_SetSanity(bool bIsActive);

#endif