#ifndef _FWCONFIG_H_
#define _FWCONFIG_H_

#define FWCONFIG_MAINTASK_PRIORITY (tskIDLE_PRIORITY + 1)
#define FWCONFIG_HTTPTASK_PRIORITY (tskIDLE_PRIORITY + 2) 

// Wi-Fi (Soft Access Point)
#define FWCONFIG_STAAP_WIFI_SSID "SBI-Iot-Svr-%02X%02X%02X"

// MDNS
#define FWCONFIG_MDNS_HOSTNAME "sbi-iot-svr"
#define FWCONFIG_MDNS_DESCRIPTION "sbi-iot-svr"
#define FWCONFIG_MDNS_SERVICENAME "sbi-iot-stove-svr"

// Options
#define FWCONFIG_SDCARD_ISACTIVE (1)
#define FWCONFIG_SDCARD_ROOTPATH "/sdcard"

// SPIFF
#define FWCONFIG_SPIFF_ROOTPATH "/spiffs"
#define FWCONFIG_SPIFF_PARTITION "storage"

#endif