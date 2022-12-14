#include <stdio.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "mdns.h"
#include "lwip/apps/netbiosns.h"
#include <esp_sntp.h>
#include "fwconfig.h"
#include "webserver.h"
#include "settings.h"
#include "main.h"
#include "espnowprocess.h"
#include "hardwaregpio.h"
#include "uartbridge.h"

#define TAG "main"

static esp_netif_t* m_pWifiSoftAP;
static esp_netif_t* m_pWifiSTA;

static void wifisoftap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void wifistation_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

static void time_sync_notification_cb(struct timeval* tv);

static void WIFI_Init()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );

    const bool isWiFiSTA = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_WSTAIsActive) == 1;
    if (isWiFiSTA)
    {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA) );
    }
    else
    {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP) );
    }

    // Access point mode
    m_pWifiSoftAP = esp_netif_create_default_wifi_ap();

    esp_netif_ip_info_t ipInfo;
    IP4_ADDR(&ipInfo.ip, 192, 168, 4, 1);
    IP4_ADDR(&ipInfo.gw, 192, 168, 4, 1);
    IP4_ADDR(&ipInfo.netmask, 255, 255, 255, 0);
    esp_netif_dhcps_stop(m_pWifiSoftAP);
    esp_netif_set_ip_info(m_pWifiSoftAP, &ipInfo);
    esp_netif_dhcps_start(m_pWifiSoftAP);

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifisoftap_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_configAP = {
        .ap = {
            .ssid = {0},
            .ssid_len = 0,
            .max_connection = 5,
        },
    };

    uint8_t macAddr[6];
    esp_read_mac(macAddr, ESP_MAC_WIFI_SOFTAP);

    sprintf((char*)wifi_configAP.ap.ssid, FWCONFIG_STAAP_WIFI_SSID, macAddr[3], macAddr[4], macAddr[5]);
    int n = strlen((const char*)wifi_configAP.ap.ssid);
    wifi_configAP.ap.ssid_len = n;

    size_t staPassLength = 64;
    NVSJSON_GetValueString(&g_sSettingHandle, SETTINGS_EENTRY_WAPPass, (char*)wifi_configAP.ap.password, &staPassLength);

    if (strlen((const char*)wifi_configAP.ap.password) == 0) 
    {
        wifi_configAP.ap.authmode = WIFI_AUTH_OPEN;
    }
    else {
        wifi_configAP.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    }

    ESP_LOGI(TAG, "SoftAP: %s", wifi_configAP.ap.ssid);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_configAP));

    if (isWiFiSTA)
    {
        m_pWifiSTA = esp_netif_create_default_wifi_sta();

        esp_event_handler_instance_t instance_any_id;
        esp_event_handler_instance_t instance_got_ip;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifistation_event_handler,
                                                            NULL,
                                                            &instance_any_id));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &wifistation_event_handler,
                                                            NULL,
                                                            &instance_got_ip));

        wifi_config_t wifi_configSTA = {
            .sta = {
                .threshold.authmode = WIFI_AUTH_WPA2_PSK,

                .pmf_cfg = {
                    .capable = true,
                    .required = false
                },
            },
        };

        size_t staSSIDLength = SETTINGS_SSID_LEN;
        NVSJSON_GetValueString(&g_sSettingHandle, SETTINGS_EENTRY_WSTASSID, (char*)wifi_configSTA.sta.ssid, &staSSIDLength);

        size_t staPassLength = SETTINGS_PASS_LEN;
        NVSJSON_GetValueString(&g_sSettingHandle, SETTINGS_EENTRY_WSTAPass, (char*)wifi_configSTA.sta.password, &staPassLength);

        ESP_LOGI(TAG, "STA mode is active, attempt to connect to ssid: %s", wifi_configSTA.sta.ssid);

        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_configSTA) );
    }

    ESP_ERROR_CHECK( esp_wifi_start());
}

static void mdns_sn_init()
{
    ESP_LOGI(TAG, "mdns_sn_init, hostname: '%s', desc: '%s', service: '%s'", 
        FWCONFIG_MDNS_HOSTNAME, 
        FWCONFIG_MDNS_DESCRIPTION, 
        FWCONFIG_MDNS_SERVICENAME);

    mdns_init();
    mdns_hostname_set(FWCONFIG_MDNS_HOSTNAME);
    mdns_instance_name_set(FWCONFIG_MDNS_DESCRIPTION);

    mdns_txt_item_t serviceTxtData[] = {
        {"funct", "sbi-iot-svr"},
        {"path", "/"}
    };

    ESP_ERROR_CHECK(mdns_service_add(FWCONFIG_MDNS_SERVICENAME, "_http", "_tcp", 80, serviceTxtData,
                                     sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));

    netbiosns_init();
    netbiosns_set_name(FWCONFIG_MDNS_HOSTNAME);
}

void MAIN_GetWiFiSTAIP(esp_netif_ip_info_t* ip)
{
    esp_netif_get_ip_info(m_pWifiSTA, ip);
}

void MAIN_GetWiFiSoftAPIP(esp_netif_ip_info_t* ip)
{
    esp_netif_get_ip_info(m_pWifiSoftAP, ip);
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    HARDWAREGPIO_Init();

    SETTINGS_Init();

    UARTBRIDGE_Init();

    WIFI_Init();

    ESPNOWPROCESS_Init();

    mdns_sn_init();

    WEBSERVER_Init();

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_init();
    
    while (true)
    {
        ESPNOWPROCESS_Handler();
        UARTBRIDGE_Handler();

        vTaskDelay(1);
    }   
}

static void wifisoftap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

static void wifistation_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();    
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        wifi_second_chan_t secondChan;
        uint8_t u8Primary;
        esp_wifi_get_channel(&u8Primary,  &secondChan);
        ESP_LOGI(TAG, "Wifi STA connected to station, channel: %d", (int)u8Primary);

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "retry to connect to the AP");
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

static void time_sync_notification_cb(struct timeval* tv)
{
    // settimeofday(tv, NULL);
    ESP_LOGI(TAG, "Notification of a time synchronization event, sec: %d", (int)tv->tv_sec);
    // Set timezone to Eastern Standard Time and print local time
    time_t now = 0;
    struct tm timeinfo = { 0 };
    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
    tzset();
    time(&now);
    localtime_r(&now, &timeinfo);
    ESP_LOGI(TAG, "The current date/time in New York is: %2d:%2d:%2d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}