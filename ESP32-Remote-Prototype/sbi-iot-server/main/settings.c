#include "settings.h"
#include <string.h>
#include "esp_log.h"

#define TAG "settings"

static bool ValidateWifiPassword(const NVSJSON_SSettingEntry* pSettingEntry, const char* szValue);
static bool ValidateESPNowRemoteMac(const NVSJSON_SSettingEntry* pSettingEntry, const char* szValue);

NVSJSON_SHandle g_sSettingHandle;

static const NVSJSON_SSettingEntry g_sConfigEntries[SETTINGS_EENTRY_Count] =
{
    // WiFi Station related   
    [SETTINGS_EENTRY_WAPPass]           = NVSJSON_INITSTRING_VAL("WAP.Pass", "WiFi password [empty, or > 8 characters]", "", ValidateWifiPassword, NVSJSON_EFLAGS_Secret | NVSJSON_EFLAGS_NeedsReboot),

    // WiFi Station related
    // DEFAULT / MIN / MAX
    [SETTINGS_EENTRY_WSTAIsActive]      = NVSJSON_INITINT32_RNG( "WSTA.IsActive", "Wifi is active", 0, 0, 1, NVSJSON_EFLAGS_NeedsReboot),
    [SETTINGS_EENTRY_WSTASSID]          = NVSJSON_INITSTRING_RNG("WSTA.SSID",    "WiFi (SSID)", "", NVSJSON_EFLAGS_NeedsReboot),
    [SETTINGS_EENTRY_WSTAPass]          = NVSJSON_INITSTRING_VAL("WSTA.Pass", "WiFi password [empty, or > 8 characters]", "", ValidateWifiPassword, NVSJSON_EFLAGS_Secret | NVSJSON_EFLAGS_NeedsReboot),

    [SETTINGS_EENTRY_ESPNowRemoteMac]   = NVSJSON_INITSTRING_VAL("ESPNOW.RemMac",    "Remote MAC Address", "00:00:00:00:00:00", ValidateESPNowRemoteMac, NVSJSON_EFLAGS_NeedsReboot),

    [SETTINGS_EENTRY_AutoSaveDebugString]= NVSJSON_INITINT32_RNG("log.savedbg", "", 60, 5, 600, NVSJSON_EFLAGS_Secret | NVSJSON_EFLAGS_NeedsReboot),
};

void SETTINGS_Init()
{
    NVSJSON_Init(&g_sSettingHandle, g_sConfigEntries, SETTINGS_EENTRY_Count);
}

static bool ValidateWifiPassword(const NVSJSON_SSettingEntry* pSettingEntry, const char* szValue)
{
    const size_t n = strlen(szValue);
    return n >= 8 || n == 0;
}

static bool ValidateESPNowRemoteMac(const NVSJSON_SSettingEntry* pSettingEntry, const char* szValue)
{
    uint8_t outMACAddr[MACADDR_LEN];
    return SETTINGS_ParseMacAddr(szValue, outMACAddr);
}

bool SETTINGS_ParseMacAddr(const char* szMacAddr, uint8_t outMACAddr[MACADDR_LEN])
{
    const size_t n = strlen(szMacAddr);
    if (n >= SETTINGS_ESPNOWREMOTEMAC_LEN)
        return false;

    int values[MACADDR_LEN];
    if( MACADDR_LEN == sscanf( szMacAddr, "%x:%x:%x:%x:%x:%x%*c",
        &values[0], &values[1], &values[2],
        &values[3], &values[4], &values[5] ) )
    {
        for(int i = 0; i < MACADDR_LEN; i++) {
            if (values[i] < 0 || values[i] > 255) {
                return false;
            }
        }

        for(int i = 0; i < MACADDR_LEN; i++) {
            outMACAddr[i] = (uint8_t)values[i];
        }
        return true;
    }

    return false;
}