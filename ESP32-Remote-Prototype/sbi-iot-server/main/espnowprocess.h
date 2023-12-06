#ifndef _ESPNOWPROCESS_H_
#define _ESPNOWPROCESS_H_

#include "SBI.iot.BaseProtocol.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_now.h"

#include <stdint.h>
#include <string.h>

#define SBI_CL 1

#define ESPNOWPROCESS_QUEUERX (5)

typedef struct struct_pairing {       // new structure for pairing
    uint8_t msgType;
    uint8_t id;
    uint8_t macAddr[6];
    uint8_t channel;
} struct_pairing;

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

typedef struct
{
    uint8_t msgType;
    uint8_t macAddr[ESP_NOW_ETH_ALEN];
    bool tStatRmt; // off, on
    uint8_t blowerSpeedRmt; // off, min, max, auto
    uint8_t distribSpeedRmt; // off, min, max, auto
    bool boostStatRmt; // off, on
    
} ESPNOWRMT_SMsg;

typedef struct
{
    uint8_t msgType;
    char time[10]; 
    float Tbaffle;
    float Tavant;
    float Plenum; 
    char State[16];
    char tStat[4];
    float dTbaffle;
    int FanSpeed;
    int Grille;
    int Prim;
    int Sec;
    float Tboard;
    char Door[8];
    uint16_t PartCH0ON;
    uint16_t PartCH1ON;
    uint16_t PartCH0OFF;
    uint16_t PartCH1OFF;
    uint16_t PartVar;
    float PartSlope;
    uint16_t TPart;
    float PartCurr;
    uint16_t PartLuxON;
    uint16_t PartLuxOFF;
    uint32_t PartTime;
    float dTavant;
    char macAddr[18];
} ESPNOWDEBUG_SMsg;


void ESPNOWPROCESS_Init();

void ESPNOWPROCESS_Handler();

ESPNOWPROCESS_ESPNowInfo ESPNOWPROCESS_GetESPNowInfo();

#endif