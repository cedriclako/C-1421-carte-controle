#include <stdio.h>
#include <esp_sntp.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "lwip/apps/netbiosns.h"
#include "fwconfig.h"
#include "spiff.h"
#include "webserver/webserver.h"
#include "settings.h"
#include "main.h"
#include "espnowprocess.h"
#include "hardwaregpio.h"
#include "uartbridge/uartbridge.h"
#include "fwconfig.h"
#include "event.h"
#include "log.h"

#define TAG "main"

ESP_EVENT_DEFINE_BASE(MAINAPP_EVENT);

static esp_netif_t* m_pWifiSoftAP;
static esp_netif_t* m_pWifiSTA;

static volatile bool m_bIsConnectedWiFi = false;
static volatile int32_t m_s32ConnectWiFiCount = 0;

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

    esp_netif_ip_info_t ipInfo = {0};
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
    // Set new priority for main task
    vTaskPrioritySet( xTaskGetCurrentTaskHandle(), FWCONFIG_MAINTASK_PRIORITY);

    HARDWAREGPIO_Init();

    SPIFF_Init();

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK( ret );

    esp_event_loop_args_t loop_args = {
        .queue_size = 20,
        .task_name = NULL
    };
    esp_event_loop_create(&loop_args, &EVENT_g_LoopHandle);

    SETTINGS_Init();

    UARTBRIDGE_Init();

    WIFI_Init();

    ESPNOWPROCESS_Init();

    WEBSERVER_Init();

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_init();

    mdns_sn_init();
    
    // Just print task list
    char* szAllTask = (char*)malloc(4096);
    vTaskList(szAllTask);
    ESP_LOGI(TAG, "vTaskList: \r\n\r\n%s", szAllTask);
    free(szAllTask);

    ESP_LOGI(TAG, "Starting ...");

    static bool isActive = false;
    TickType_t ttSanityLed = xTaskGetTickCount();

    // Run main loop at 200 hz
    const int loopPeriodMS = 1000/200;
    const TickType_t xFrequency = loopPeriodMS / portTICK_PERIOD_MS;

    LOG_Init();

    while (true)
    {
        TickType_t xLastWakeTime = xTaskGetTickCount();

        // Basic processes ...
        ESPNOWPROCESS_Handler();
        UARTBRIDGE_Handler();
        LOG_Handler();

        // Sanity LED process
        if ( (xTaskGetTickCount() - ttSanityLed) > pdMS_TO_TICKS(m_bIsConnectedWiFi ? 150 : 500) )
        {
            ttSanityLed = xTaskGetTickCount();
            HARDWAREGPIO_SetSanity(isActive);
            isActive = !isActive;
        }

       //vTaskDelay(10);
       esp_event_loop_run(EVENT_g_LoopHandle, pdMS_TO_TICKS( 1 ));
       vTaskDelayUntil( &xLastWakeTime, xFrequency );
    }   
}

static void wifisoftap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%"PRId32,
                 MAC2STR(event->mac), (int32_t)event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%"PRId32,
                 MAC2STR(event->mac), (int32_t)event->aid);
    }
}

static void wifistation_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();    
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        m_bIsConnectedWiFi = true;
        wifi_second_chan_t secondChan;
        uint8_t u8Primary;
        esp_wifi_get_channel(&u8Primary,  &secondChan);
        ESP_LOGI(TAG, "Wifi STA connected to station, channel: %"PRId32, (int32_t)u8Primary);

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        m_bIsConnectedWiFi = false;
        esp_wifi_connect();
        ESP_LOGI(TAG, "connect to the AP fail, retry to connect to the AP, attempt: #%"PRId32, (int32_t)++m_s32ConnectWiFiCount);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

static void time_sync_notification_cb(struct timeval* tv)
{
    // settimeofday(tv, NULL);
    ESP_LOGI(TAG, "Notification of a time synchronization event, sec: %"PRId32, (int32_t)tv->tv_sec);
    // Set timezone to Eastern Standard Time and print local time
    time_t now = 0;
    struct tm timeinfo = { 0 };
    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
    tzset();
    time(&now);
    localtime_r(&now, &timeinfo);
    ESP_LOGI(TAG, "The current date/time in New York is: %2"PRId32":%2"PRId32":%2"PRId32, (int32_t)timeinfo.tm_hour, (int32_t)timeinfo.tm_min, (int32_t)timeinfo.tm_sec);
}