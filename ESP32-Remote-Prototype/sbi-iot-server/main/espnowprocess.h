#ifndef _ESPNOWPROCESS_H_
#define _ESPNOWPROCESS_H_

#include "SBI.iot.BaseProtocol.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_now.h"

#include <stdint.h>
#include <string.h>

#define ESPNOWPROCESS_QUEUERX (5)

typedef struct
{
    volatile uint32_t u32RX;
    volatile uint32_t u32TX;
} ESPNOWPROCESS_ESPNowInfo;

typedef struct
{
    uint8_t u8SrcMACs[ESP_NOW_ETH_ALEN];

    uint8_t u8Buffers[SBIIOTBASEPROTOCOL_MAXPAYLOADLEN];
    uint8_t u8BufferCount;
} ESPNOWPROCESS_SMsg;


void ESPNOWPROCESS_Init();

void ESPNOWPROCESS_Handler();

ESPNOWPROCESS_ESPNowInfo ESPNOWPROCESS_GetESPNowInfo();

#endif