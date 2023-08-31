#ifndef _STM32PROCESS_H_
#define _STM32PROCESS_H_

#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"

#include "stm32-protocol.h"

esp_err_t STM32PROCESS_FlashSTM(const STM32PROTOCOL_SContext* pContext, const char* filename);

#endif