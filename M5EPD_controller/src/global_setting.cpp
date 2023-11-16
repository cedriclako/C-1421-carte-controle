#include "global_setting.h"
#include "./resources/ImageResource.h"
#include "esp32-hal-log.h"
#include <WiFi.h>

#define DEFAULT_WALLPAPER 2
SemaphoreHandle_t _xSemaphore_LoadingAnime = NULL;
static uint8_t _loading_anime_eixt_flag    = false;
esp_err_t __espret__;
#define NVS_CHECK(x)            \
    __espret__ = x;             \
    if (__espret__ != ESP_OK) { \
        nvs_close(nvs_arg);     \
        log_e("Check Err");     \
        return __espret__;      \
    }

const uint8_t *wallpapers[] = {
    ImageResource_wallpaper_engine_540x960,
    ImageResource_wallpaper_engine_540x960,
    ImageResource_wallpaper_engine_540x960};

const uint8_t *kIMGLoading[16] = {
    ImageResource_item_loading_01_32x32, ImageResource_item_loading_02_32x32,
    ImageResource_item_loading_03_32x32, ImageResource_item_loading_04_32x32,
    ImageResource_item_loading_05_32x32, ImageResource_item_loading_06_32x32,
    ImageResource_item_loading_07_32x32, ImageResource_item_loading_08_32x32,
    ImageResource_item_loading_09_32x32, ImageResource_item_loading_10_32x32,
    ImageResource_item_loading_11_32x32, ImageResource_item_loading_12_32x32,
    ImageResource_item_loading_13_32x32, ImageResource_item_loading_14_32x32,
    ImageResource_item_loading_15_32x32, ImageResource_item_loading_16_32x32};
const char *wallpapers_name_en[] = {"M5Paper", "Engine", "Penrose Triangle"};
const char *wallpapers_name_zh[] = {"M5Paper", "引擎", "彭罗斯三角"};
const char *wallpapers_name_ja[] = {"M5Paper", "エンジン",
                                    "ペンローズの三角形"};
uint16_t global_wallpaper       = DEFAULT_WALLPAPER;
uint8_t global_language         = LANGUAGE_EN;
uint8_t global_degreesunit      = DEGREES_FAHRENHEIT;
uint8_t global_thermostatmode   = AUTOMATIC;
uint8_t global_tstatvalue       = 0;
String global_wifi_ssid;
String global_wifi_password;
uint8_t global_wifi_configed    = false;
uint16_t global_reader_textsize = 32;
uint8_t global_time_synced      = false;
uint8_t global_ttf_file_loaded  = false;
uint8_t global_init_status      = 0xFF;
int8_t global_timezone          = 8;
uint16_t global_setpoint        = 215; // 22.5 * 10
uint32_t global_voltagebattery  = 0;
uint8_t global_switchstate      = SWITCH_LOW;
uint8_t global_blowerspeed      = 0;
uint8_t global_distribspeed     = 0;
uint8_t global_booststat       = 0;


int8_t GetTimeZone(void) {
    return global_timezone;
}

void SetTimeZone(int8_t time_zone) {
    global_timezone = time_zone;
}

void SetInitStatus(uint8_t idx, uint8_t val) {
    global_init_status &= ~(1 << idx);
    global_init_status |= (val << idx);
}

uint8_t GetInitStatus(uint8_t idx) {
    return (global_init_status & (1 << idx)) ? true : false;
}

void SetTTFLoaded(uint8_t val) {
    global_ttf_file_loaded = val;
}

uint8_t isTTFLoaded() {
    return global_ttf_file_loaded;
}

uint8_t isTimeSynced(void) {
    return global_time_synced;
}

void SetTimeSynced(uint8_t val) {
    global_time_synced = val;
    SaveSetting();
}




void SetBlowerSpeed(uint8_t blowerspeed) {
    global_blowerspeed = blowerspeed;
    SaveSetting();
}

uint8_t GetBlowerSpeed(void) {
    return global_blowerspeed;
}

void SetDistributionSpeed(uint8_t distribspeed) {
    global_distribspeed = distribspeed;
    SaveSetting();
}

uint8_t GetDistributionSpeed(void) {
    return global_distribspeed;
}

void SetBoostStat(uint8_t booststat) {
    global_booststat = booststat;
    SaveSetting();
}

uint8_t GetBoostStat(void) {
    return global_booststat;
}

void SetLanguage(uint8_t language) {
    if (language >= LANGUAGE_EN && language <= LANGUAGE_FR) {
        global_language = language;
    }
    SaveSetting();
}

uint8_t GetLanguage(void) {
    return global_language;
}

void SetDegreesUnit(uint8_t degreesunit) {
    global_degreesunit = degreesunit;
    SaveSetting();
}

uint8_t GetDegreesUnit(void) {
    return global_degreesunit;
}

void SetThermostatMode(uint8_t thermostatmode) {
    global_thermostatmode = thermostatmode;
    SaveSetting();
}

uint8_t GetThermostatMode(void) {
    return global_thermostatmode;
}

void SetThermostatValue(uint8_t tstatvalue) {
    global_tstatvalue = tstatvalue;
    SaveSetting();
}

uint8_t GetThermostatValue(void) {
    return global_tstatvalue;
}

void SetSetPoint(uint16_t setpoint) {
    global_setpoint = setpoint;
    SaveSetting();
}

uint16_t GetSetpoint(void) {
    return global_setpoint;
}

void SetVoltageBattery(uint32_t voltagebattery) {
    global_voltagebattery = voltagebattery;
    SaveSetting();
}

uint32_t GetVoltageBattery(void) {
    return global_voltagebattery;
}

void SetSwitchState(uint8_t switchstate) {
    global_switchstate = switchstate;
    SaveSetting();
}

uint8_t GetSwitchState(void) {
    return global_switchstate;
}

esp_err_t LoadSetting(void) {
    nvs_handle nvs_arg;
    NVS_CHECK(nvs_open("Setting", NVS_READONLY, &nvs_arg));

#if 0
    NVS_CHECK(nvs_get_u8(nvs_arg, "Language", &global_language));
    
    size_t length = 128;
    char buf[128];
    NVS_CHECK(nvs_get_str(nvs_arg, "ssid", buf, &length));
    global_wifi_ssid = String(buf);
    length           = 128;
    NVS_CHECK(nvs_get_str(nvs_arg, "pswd", buf, &length));
    global_wifi_password = String(buf);
    global_wifi_configed = true;
#else
    NVS_CHECK(nvs_get_u8(nvs_arg, "language", &global_language));
    NVS_CHECK(nvs_get_u16(nvs_arg, "setpoint", &global_setpoint));
    NVS_CHECK(nvs_get_u8(nvs_arg, "tstatmode", &global_thermostatmode));
    NVS_CHECK(nvs_get_u8(nvs_arg, "degreesunit", &global_degreesunit));
    NVS_CHECK(nvs_get_u8(nvs_arg, "switchstate", &global_switchstate));
    NVS_CHECK(nvs_get_u8(nvs_arg, "tstatval", &global_tstatvalue));
    NVS_CHECK(nvs_get_u8(nvs_arg, "blowerspeed", &global_blowerspeed));
    NVS_CHECK(nvs_get_u8(nvs_arg, "distribspeed", &global_distribspeed));
    NVS_CHECK(nvs_get_u8(nvs_arg, "booststat", &global_booststat));

#endif
    nvs_close(nvs_arg);
    return ESP_OK;
}

esp_err_t SaveSetting(void) {
    nvs_handle nvs_arg;
    NVS_CHECK(nvs_open("Setting", NVS_READWRITE, &nvs_arg));

#if 0
    
    NVS_CHECK(nvs_set_u8(nvs_arg, "Language", global_language));
    
    NVS_CHECK(nvs_set_str(nvs_arg, "ssid", global_wifi_ssid.c_str()));
    NVS_CHECK(nvs_set_str(nvs_arg, "pswd", global_wifi_password.c_str()));
#else
    NVS_CHECK(nvs_set_u8(nvs_arg, "language", global_language));
    NVS_CHECK(nvs_set_u16(nvs_arg, "setpoint", global_setpoint));
    NVS_CHECK(nvs_set_u8(nvs_arg, "tstatmode", global_thermostatmode));
    NVS_CHECK(nvs_set_u8(nvs_arg, "degreesunit", global_degreesunit));
    NVS_CHECK(nvs_set_u8(nvs_arg, "switchstate", global_switchstate));
    NVS_CHECK(nvs_set_u8(nvs_arg, "tstatval", global_tstatvalue));
    NVS_CHECK(nvs_set_u8(nvs_arg, "blowerspeed", global_blowerspeed));
    NVS_CHECK(nvs_set_u8(nvs_arg, "distribspeed", global_distribspeed));
    NVS_CHECK(nvs_set_u8(nvs_arg, "booststat", global_booststat));

#endif

    NVS_CHECK(nvs_commit(nvs_arg));
    nvs_close(nvs_arg);
    return ESP_OK;
}

void SetWifi(String ssid, String password) {
    global_wifi_ssid     = ssid;
    global_wifi_password = password;
    SaveSetting();
}

uint8_t isWiFiConfiged(void) {
    return global_wifi_configed;
}

String GetWifiSSID(void) {
    return global_wifi_ssid;
}

String GetWifiPassword(void) {
    return global_wifi_password;
}

uint16_t GetTextSize() {
    return global_reader_textsize;
}

void SetTextSize(uint16_t size) {
    global_reader_textsize = size;
}

const uint8_t *GetLoadingIMG_32x32(uint8_t id) {
    return kIMGLoading[id];
}

void __LoadingAnime_32x32(void *pargs) {
    uint16_t *args = (uint16_t *)pargs;
    uint16_t x     = args[0];
    uint16_t y     = args[1];
    free(pargs);
    M5EPD_Canvas loading(&M5.EPD);
    loading.createCanvas(32, 32);
    loading.fillCanvas(0);
    loading.pushCanvas(x, y, UPDATE_MODE_GL16);
    int anime_cnt = 0;
    uint32_t time = 0;
    while (1) {
        if (millis() - time > 200) {
            time = millis();
            loading.pushImage(0, 0, 32, 32, GetLoadingIMG_32x32(anime_cnt));
            loading.pushCanvas(x, y, UPDATE_MODE_DU4);
            anime_cnt++;
            if (anime_cnt == 16) {
                anime_cnt = 0;
            }
        }

        xSemaphoreTake(_xSemaphore_LoadingAnime, portMAX_DELAY);
        if (_loading_anime_eixt_flag == true) {
            xSemaphoreGive(_xSemaphore_LoadingAnime);
            break;
        }
        xSemaphoreGive(_xSemaphore_LoadingAnime);
    }
    vTaskDelete(NULL);
}

void LoadingAnime_32x32_Start(uint16_t x, uint16_t y) {
    if (_xSemaphore_LoadingAnime == NULL) {
        _xSemaphore_LoadingAnime = xSemaphoreCreateMutex();
    }
    _loading_anime_eixt_flag = false;
    uint16_t *pos            = (uint16_t *)calloc(2, sizeof(uint16_t));
    pos[0]                   = x;
    pos[1]                   = y;
    xTaskCreatePinnedToCore(__LoadingAnime_32x32, "__LoadingAnime_32x32",
                            16 * 1024, pos, 1, NULL, 0);
}

void LoadingAnime_32x32_Stop() {
    xSemaphoreTake(_xSemaphore_LoadingAnime, portMAX_DELAY);
    _loading_anime_eixt_flag = true;
    xSemaphoreGive(_xSemaphore_LoadingAnime);
    delay(200);
}

void Shutdown() {
    log_d("Now the system is shutting down.");
    M5.EPD.Clear();
    M5.EPD.WritePartGram4bpp(92, 182, 356, 300, ImageResource_logo_356x300);
    M5.EPD.UpdateFull(UPDATE_MODE_GC16);
    M5.EPD.UpdateFull(UPDATE_MODE_GC16);
    SaveSetting();
    delay(600);
    M5.disableEPDPower();
    M5.disableEXTPower();
    M5.disableMainPower();
    esp_deep_sleep_start();
    while (1);
}