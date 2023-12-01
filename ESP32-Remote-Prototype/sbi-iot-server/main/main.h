#ifndef _MAIN_H_
#define _MAIN_H_

#include "esp_netif_ip_addr.h"
#include "esp_system.h"
#include "esp_wifi.h"


void MAIN_GetWiFiSTAIP(esp_netif_ip_info_t* ip);

int32_t MAIN_GetWiFiSTAIPv6(esp_ip6_addr_t if_ip6[CONFIG_LWIP_IPV6_NUM_ADDRESSES]);

void MAIN_GetWiFiSoftAPIP(esp_netif_ip_info_t* ip);

void removeChar(char *str, char c);

#endif