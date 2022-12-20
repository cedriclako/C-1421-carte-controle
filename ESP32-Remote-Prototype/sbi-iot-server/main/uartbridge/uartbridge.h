#ifndef _UARTBRIDGE_H_
#define _UARTBRIDGE_H_

#include <stdint.h>
#include "ufec23_protocol.h"

#define UARTBRIDGE_COMMUNICATIONLOST_TIMEOUT_MS (1000)
#define UARTBRIDGE_KEEPALIVE_MS (250)

#define UARTBRIDGE_PROCDOWNLOADUPLOAD_MS (5000)

void UARTBRIDGE_Init();

void UARTBRIDGE_SendFrame(UFEC23PROTOCOL_FRAMEID eFrameID, uint8_t u8Payloads[], uint16_t u16PayloadLen);

void UARTBRIDGE_Handler();

#endif