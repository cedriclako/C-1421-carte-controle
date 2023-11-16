#ifndef _GLOBAL_SETTING_H_
#define _GLOBAL_SETTING_H_

#include <M5EPD.h>
#include <nvs.h>

#define WALLPAPER_NUM 3

// 是否开启自动关机省电
#define ENABLE_AUTO_POWER_SAVE true

static const uint32_t TIME_UPDATE_BATTERY_VOLTAGE_MS = 60 * 1000;

static const uint32_t TIME_UPDATE_ROOM_MS = 60 * 1000;

static const uint32_t TIME_BEFORE_SHUTDOWN_PROMPT_MS = 60 * 1000;

static const uint32_t SHUTDOWN_PROMPT_DELAY_MS = 10 * 1000;

static const uint32_t TIME_BEFORE_SHUTDOWN_MS =
    TIME_BEFORE_SHUTDOWN_PROMPT_MS + SHUTDOWN_PROMPT_DELAY_MS;

enum {
    LANGUAGE_EN = 0,  // default, English
    LANGUAGE_FR,      // Frensh
};

enum {
    DEGREES_FAHRENHEIT = 0,      // default, Fahrenheit
    DEGREES_CELSIUS,  // Celsius
    
};

enum {
    AUTOMATIC = 0,  // default, automatic
    MANUAL,      // Manual
};

enum {
    SWITCH_LOW = 0,  // default, Low
    SWITCH_HIGH,      // High
};

void SetBlowerSpeed(uint8_t blowerspeed);
uint8_t GetBlowerSpeed(void);
void SetDistributionSpeed(uint8_t distribspeed);
uint8_t GetDistributionSpeed(void);
void SetBoostStat(uint8_t booststat);
uint8_t GetBoostStat(void);
void SetLanguage(uint8_t language);
uint8_t GetLanguage(void);
void SetDegreesUnit(uint8_t degreesunit);
uint8_t GetDegreesUnit(void);
void SetThermostatMode(uint8_t thermostatmode);
uint8_t GetThermostatMode(void);
void SetThermostatValue(uint8_t tstatvalue);
uint8_t GetThermostatValue(void);
void SetSetPoint(uint16_t setpoint);
uint16_t GetSetpoint(void);

void SetVoltageBattery(uint32_t voltagebattery);
uint32_t GetVoltageBattery(void);

void SetSwitchState(uint8_t switchstate);
uint8_t GetSwitchState(void);

esp_err_t LoadSetting(void);
esp_err_t SaveSetting(void);
void SetWifi(String ssid, String password);
String GetWifiSSID(void);
String GetWifiPassword(void);
uint8_t isWiFiConfiged(void);

int8_t GetTimeZone(void);
void SetTimeZone(int8_t time_zone);

uint16_t GetTextSize();
void SetTextSize(uint16_t size);

const uint8_t* GetLoadingIMG_32x32(uint8_t id);
void LoadingAnime_32x32_Start(uint16_t x, uint16_t y);
void LoadingAnime_32x32_Stop();

uint8_t isTimeSynced(void);
void SetTimeSynced(uint8_t val);

void SetTTFLoaded(uint8_t val);
uint8_t isTTFLoaded(void);
void SetInitStatus(uint8_t idx, uint8_t val);
uint8_t GetInitStatus(uint8_t idx);
void Shutdown();

#endif  //_GLOBAL_SETTING_H_