#ifndef _UARTBRIDGE_H_
#define _UARTBRIDGE_H_

#include <stdint.h>

void UARTBRIDGE_Init();

void UARTBRIDGE_SendFrame(uint8_t u8Payloads, uint32_t u8PayloadLen);

void UARTBRIDGE_Handler();

#endif