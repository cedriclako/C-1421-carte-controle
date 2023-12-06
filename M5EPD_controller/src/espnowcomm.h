#ifndef _ESPNOWCOMM_H_
#define _ESPNOWCOMM_H_

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_now.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESPNOWCOMM_QUEUERX (5)

#define ESPNOWCOMM_SCANCHANNEL_TIMEOUTPERCHAN_MS (50)
#define ESPNOWCOMM_GETSTATUS_RETRY_MS (100)

#define ESPNOW_PMK "pmk1234567890123"

#define MAX_CHANNEL 12  // for North America // 13 in Europe
#define BOARD_ID 1

enum PairingStatus {NOT_PAIRED, PAIR_REQUEST, PAIR_REQUESTED, PAIR_PAIRED,};

typedef struct struct_pairing {       // new structure for pairing
    uint8_t msgType;
    uint8_t id;
    uint8_t macAddr[6];
    uint8_t channel;
} struct_pairing;




typedef struct
{
    uint8_t msgType;
    uint8_t macAddr[6];
    bool tStatRmt; // off, on
    uint8_t blowerSpeedRmt; // 0, 1, 2
    uint8_t distribSpeedRmt; // 0, 1, 2
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

void configDeviceAP(String macAddrAP);

void InitESPNow();

void ConfigBrodcastESPNow();

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

void sendDataToEspNow(ESPNOWRMT_SMsg data);

void addPeer(const uint8_t * mac_addr, uint8_t chan);

void printMAC(const uint8_t * mac_addr);

PairingStatus autoPairing();

extern ESPNOWDEBUG_SMsg espNowDataRcv;
extern ESPNOWRMT_SMsg espNowDataSent;
extern ESPNOWDEBUG_SMsg dataDebug;
extern int SlaveCnt;

#ifdef __cplusplus
}
#endif

#endif