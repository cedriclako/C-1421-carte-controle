#include <stdio.h>
#include <esp_sntp.h>
#include <inttypes.h>
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
#include "OTACheck.h"
#include "spiff.h"
#include "webserver/webserver.h"
#include "settings.h"
#include "main.h"
#include "espnowprocess.h"
#include "hardwaregpio.h"
#include "uartbridge/uartbridge.h"
#include "iot/IoTBridge.h"
#include "fwconfig.h"
#include "event.h"
#include "log.h"
#include "esp_ota_ops.h"
#include <sys/param.h>
#include "esp_tls.h"
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_http_client.h"

#include <sys/param.h>
#include "esp_tls.h"
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif

#include "freertos/task.h"
#include "esp_system.h"
#include "esp_http_client.h"

#define TAG "main"

ESP_EVENT_DEFINE_BASE(MAINAPP_EVENT);

static esp_netif_t* m_pWifiSoftAP = NULL;
static esp_netif_t* m_pWifiSTA = NULL;

static volatile bool m_bIsConnectedWiFi = false;
static volatile int32_t m_s32ConnectWiFiCount = 0;

static void wifisoftap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void wifistation_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

static void time_sync_notification_cb(struct timeval* tv);
static void CheckForOTAUpdate();

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
        esp_event_handler_instance_t instance_got_ip6;
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
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_GOT_IP6,
                                                            &wifistation_event_handler,
                                                            NULL,
                                                            &instance_got_ip6));

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

bool MAIN_GetIsWiFiConnected()
{
    return m_bIsConnectedWiFi;
}

void MAIN_GetWiFiSTAIP(esp_netif_ip_info_t* ip)
{
    esp_netif_get_ip_info(m_pWifiSTA, ip);
}

int32_t MAIN_GetWiFiSTAIPv6(esp_ip6_addr_t if_ip6[CONFIG_LWIP_IPV6_NUM_ADDRESSES])
{
    if (m_pWifiSTA != NULL)
        return esp_netif_get_all_ip6(m_pWifiSTA, if_ip6);
    return 0;
}

void MAIN_GetWiFiSoftAPIP(esp_netif_ip_info_t* ip)
{
    esp_netif_get_ip_info(m_pWifiSoftAP, ip);
}

static void CheckForOTAUpdate()
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) 
    {
        // Check if it's a new update
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) 
        {
            // run diagnostic function ...
            const bool diagnostic_is_ok = true;
            if (diagnostic_is_ok) 
            {
                // TODO: That's where we will start the update process for the UFEC23 (STM32)
                // Just start another task and wait
                // don't forget to kick the watchdog
                ESP_LOGI(TAG, "Diagnostics completed successfully! Continuing execution ....");
                esp_ota_mark_app_valid_cancel_rollback();
            }
            else
            {
                ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ....");
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
    }
}

void removeChar(char *str, char c) {
    int i, j;
    int len = strlen(str);
    for (i = j = 0; i < len; i++) {
        if (str[i] != c) {
            str[j++] = str[i];
        }
    }
    str[j] = '\0';
}

void app_main(void)
{
    // Set new priority for main task
    vTaskPrioritySet( xTaskGetCurrentTaskHandle(), FWCONFIG_MAINTASK_PRIORITY);

    // Check if an update is pending
    CheckForOTAUpdate();
    
    HARDWAREGPIO_Init();

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }

    SPIFF_Init();

    ESP_ERROR_CHECK( ret );

    esp_event_loop_args_t loop_args = {
        .queue_size = 20,
        .task_name = NULL
    };
    esp_event_loop_create(&loop_args, &EVENT_g_LoopHandle);

    SETTINGS_Init();
    OTACHECK_Init();
    UARTBRIDGE_Init();
    WIFI_Init();
    ESPNOWPROCESS_Init();
    WEBSERVER_Init();
    IOTBRIDGE_Init();

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_init();
    mdns_sn_init();
    
    ESP_LOGI(TAG, "Starting ...");

    static bool isActive = false;
    TickType_t ttSanityLed = xTaskGetTickCount();

    // Run main loop at 200 hz
    const int loopPeriodMS = 1000/200;
    const TickType_t xFrequency = loopPeriodMS / portTICK_PERIOD_MS;

    LOG_Init();
    UARTBRIDGE_Start();
    IOTBRIDGE_Start();

    // Just print the task list, for debug purposes
    char* szAllTask = (char*)malloc(4096);
    vTaskList(szAllTask);
    ESP_LOGI(TAG, "vTaskList: \r\n\r\n%s", szAllTask);
    free(szAllTask);

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
        esp_netif_create_ip6_linklocal(m_pWifiSTA);

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        m_bIsConnectedWiFi = false;
        esp_wifi_connect();
        ESP_LOGI(TAG, "connect to the AP fail, retry to connect to the AP, attempt: #%"PRId32, (int32_t)++m_s32ConnectWiFiCount);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_GOT_IP6) {
        ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
        ESP_LOGI(TAG, "Got IPv6 address " IPV6STR, IPV62STR(event->ip6_info.ip));
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