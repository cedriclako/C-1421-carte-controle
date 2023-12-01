#include <M5EPD.h>

#include "esp_wifi.h"
#include "esp_log.h"

#include "espnowcomm.h"
#include <WiFi.h>
#include "cJSON.h"

#define TAG "ESPNOWCOMM"


ESPNOWDEBUG_SMsg espNowDataRcv;
ESPNOWRMT_SMsg espNowDataSent;
ESPNOWDEBUG_SMsg dataDebug;
char macAddrRmt[18];

static const uint8_t m_u8BroadcastAddr[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// config AP SSID
void configDeviceAP(String macAddrAP) {
  String subMacAddrAP = macAddrAP;
  subMacAddrAP.replace(":", "");
  String SSID = "SbiRmt" + subMacAddrAP;
  bool result = WiFi.softAP(SSID.c_str(), NULL, CHANNEL, 0);
  if (!result) {
    log_d("AP Config failed.");
  } else {
    log_d("AP Config Success. Broadcasting with AP: %s", String(SSID));
    log_d("AP CHANNEL %d", WiFi.channel());
  }
 
  memset(macAddrRmt, '\0', sizeof(macAddrRmt));
  strcpy(macAddrRmt, macAddrAP.c_str());
}

// Init ESP Now with fallback
void InitESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    log_d("ESPNow Init Success");
  }
  else {
    log_d("ESPNow Init Failed");
    // Retry InitESPNow, add a counte and then restart?
    // InitESPNow();
    // or Simply Restart
    ESP.restart();
  }
}

void ConfigBrodcastESPNow() {
    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t peer;
    memset(&peer, 0, sizeof(esp_now_peer_info_t));
    peer.channel = CHANNEL; //u8CurrentChannel;
    peer.ifidx = WIFI_IF_AP;
    peer.encrypt = false;
    memcpy(peer.peer_addr, m_u8BroadcastAddr, ESP_NOW_ETH_ALEN);
    esp_now_add_peer(&peer);
}

// callback when data is recv from Master
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  log_d("Last Packet Recv from: %s", macStr);
  
  memcpy(&espNowDataRcv, data, sizeof(espNowDataRcv));
  for (size_t i = 0; i < strlen(espNowDataRcv.macAddr); ++i) {
    espNowDataRcv.macAddr[i] = tolower(espNowDataRcv.macAddr[i]);
  }
  log_d("espNowDataRcv.macAddr: %s",espNowDataRcv.macAddr);
  log_d("macAddrRmt: %s",macAddrRmt);
  
  if(!strcmp(espNowDataRcv.macAddr, macAddrRmt))
  {
    log_d("dataDebug.time: %s",espNowDataRcv.time);
    log_d("dataDebug.Tbaffle: %.2f",espNowDataRcv.Tbaffle);
    log_d("dataDebug.Tavant: %.2f",espNowDataRcv.Tavant);
    log_d("dataDebug.Plenum: %.2f",espNowDataRcv.Plenum);
    log_d("dataDebug.State: %s",espNowDataRcv.State);
    log_d("dataDebug.tStat: %s",espNowDataRcv.tStat);

    dataDebug = espNowDataRcv;
  }
  
}

// callback when data is sent from Master to Slave
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  log_d("Last Packet Sent to: %s", macStr);
  log_d("Last Packet Send Status: %s",status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// send data
void sendDataToEspNow(ESPNOWRMT_SMsg data) {

  esp_err_t result = esp_now_send(m_u8BroadcastAddr, (const uint8_t *) &data, sizeof(data));
  log_d("espNowDataSent.tStatRmt: %d",data.tStatRmt);
  log_d("espNowDataSent.blowerSpeedRmt: %d",data.blowerSpeedRmt);
  log_d("espNowDataSent.distribSpeedRmt: %d",data.distribSpeedRmt);
  log_d("espNowDataSent.boostStatRmt: %d",data.boostStatRmt);

  if (result == ESP_OK) {
    log_d("Send Status: Success");
  } else if (result == ESP_ERR_ESPNOW_NOT_INIT) {
    // How did we get so far!!
    log_d("Send Status: ESPNOW not Init.");
  } else if (result == ESP_ERR_ESPNOW_ARG) {
    log_d("Send Status: Invalid Argument");
  } else if (result == ESP_ERR_ESPNOW_INTERNAL) {
    log_d("Send Status: Internal Error");
  } else if (result == ESP_ERR_ESPNOW_NO_MEM) {
    log_d("Send Status: ESP_ERR_ESPNOW_NO_MEM");
  } else if (result == ESP_ERR_ESPNOW_NOT_FOUND) {
    log_d("Send Status: Peer not found.");
  } else {
    log_d("Send Status: Not sure what happened");
  }
}

