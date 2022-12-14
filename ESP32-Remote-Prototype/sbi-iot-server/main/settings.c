#include "settings.h"
#include <string.h>

static bool ValidateWifiPassword(const NVSJSON_SSettingEntry* pSettingEntry, const char* szValue);

NVSJSON_SHandle g_sSettingHandle;

static const NVSJSON_SSettingEntry g_sConfigEntries[SETTINGS_EENTRY_Count] =
{
    // WiFi Station related   
    [SETTINGS_EENTRY_WAPPass]      = NVSJSON_INITSTRING_VAL("WAP.Pass", "WiFi password [empty, or > 8 characters]", "", ValidateWifiPassword, NVSJSON_EFLAGS_Secret | NVSJSON_EFLAGS_NeedsReboot),

    // WiFi Station related
    [SETTINGS_EENTRY_WSTAIsActive] = NVSJSON_INITINT32_RNG( "WSTA.IsActive", "Wifi is active", 0, 0, 1, NVSJSON_EFLAGS_NeedsReboot),
    [SETTINGS_EENTRY_WSTASSID]     = NVSJSON_INITSTRING_RNG("WSTA.SSID",    "WiFi (SSID)", "", NVSJSON_EFLAGS_NeedsReboot),
    [SETTINGS_EENTRY_WSTAPass]     = NVSJSON_INITSTRING_VAL("WSTA.Pass", "WiFi password [empty, or > 8 characters]", "", ValidateWifiPassword, NVSJSON_EFLAGS_Secret | NVSJSON_EFLAGS_NeedsReboot),


};

void SETTINGS_Init()
{
    NVSJSON_Init(&g_sSettingHandle, g_sConfigEntries, SETTINGS_EENTRY_Count);
}

static bool ValidateWifiPassword(const NVSJSON_SSettingEntry* pSettingEntry, const char* szValue)
{
    const size_t n = strlen(szValue);
    return n > 8 || n == 0;
}