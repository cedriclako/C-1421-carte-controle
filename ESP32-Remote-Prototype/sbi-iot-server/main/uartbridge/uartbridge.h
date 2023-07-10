#ifndef _UARTBRIDGE_H_
#define _UARTBRIDGE_H_

#include <stdint.h>
#include "ufec23_protocol.h"

#define UARTBRIDGE_COMMUNICATIONLOST_TIMEOUT_MS (2000)
#define UARTBRIDGE_KEEPALIVE_MS (500)

#define UARTBRIDGE_PROCDOWNLOADUPLOAD_MS (5000)

void UARTBRIDGE_Init();

void UARTBRIDGE_SetSilenceMode(bool bIsSilent);

void UARTBRIDGE_Handler();


#endif