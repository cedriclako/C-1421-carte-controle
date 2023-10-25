#ifndef _SETTING_H_
#define _SETTING_H_

#include "nvsjson.h"
#include "esp_now.h"

typedef enum
{
    SETTINGS_EENTRY_WAPPass,

    SETTINGS_EENTRY_WSTAIsActive,
    SETTINGS_EENTRY_WSTASSID,
    SETTINGS_EENTRY_WSTAPass,

    SETTINGS_EENTRY_ESPNowRemoteMac,

    SETTINGS_EENTRY_AutoSaveDebugString,

    SETTINGS_EENTRY_Count
} SETTINGS_EENTRY;

#define SETTINGS_SSID_LEN 32
#define SETTINGS_PASS_LEN 64

#define SETTINGS_ESPNOWREMOTEMAC_LEN 20

#define MACADDR_LEN (ESP_NOW_ETH_ALEN)

void SETTINGS_Init();

bool SETTINGS_ParseMacAddr(const char* szMacAddr, uint8_t outMACAddr[6]);

extern NVSJSON_SHandle g_sSettingHandle;

#endif