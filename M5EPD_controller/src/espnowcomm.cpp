#include <M5EPD.h>

#include "esp_wifi.h"
#include "esp_log.h"

#include "espnowcomm.h"
#include <WiFi.h>
#include "cJSON.h"

#define TAG "ESPNOWCOMM"

struct_pairing pairingData;


PairingStatus pairingStatus = PAIR_REQUEST; //NOT_PAIRED;

enum MessageType {PAIRING, DATA,};


#ifdef SAVE_CHANNEL
  int lastChannel;
#endif  
int channel = 1;

unsigned long currentMillis = millis();
unsigned long previousMillis = 0;   // Stores last time temperature was published

ESPNOWDEBUG_SMsg espNowDataRcv;
ESPNOWRMT_SMsg espNowDataSent;
ESPNOWDEBUG_SMsg dataDebug;
char macAddrRmt[18];

uint8_t m_u8BroadcastAddr[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// config AP SSID
void configDeviceAP(String macAddrAP) {
  String subMacAddrAP = macAddrAP;
  subMacAddrAP.replace(":", "");
  String SSID = "SbiRmt" + subMacAddrAP;
  bool result = WiFi.softAP(SSID.c_str(), NULL, channel, 0);
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
    peer.channel = channel; //u8CurrentChannel;
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
  
#if 0

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

#endif

  uint8_t type = data[0];
  switch (type) {
    case DATA :      // we received data from server
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
      break;

    case PAIRING:    // we received pairing data from server
      memcpy(&pairingData, data, sizeof(pairingData));
      if (pairingData.id == 0) {              // the message comes from server
        printMAC(mac_addr);
        log_d("Pairing done for ");
        printMAC(pairingData.macAddr);
        log_d(" on channel %d", pairingData.channel); // channel used by the server

        addPeer(pairingData.macAddr, pairingData.channel); // add the server  to the peer list 
        #ifdef SAVE_CHANNEL
          lastChannel = pairingData.channel;
          EEPROM.write(0, pairingData.channel);
          EEPROM.commit();
        #endif  
        pairingStatus = PAIR_PAIRED;             // set the pairing status
      }
      break;
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

data.msgType = DATA;
  esp_err_t result = esp_now_send(m_u8BroadcastAddr, (const uint8_t *) &data, sizeof(data));
  log_d("espNowDataSent.macAddr: %02x:%02x:%02x:%02x:%02x:%02x", data.macAddr[0], data.macAddr[1], data.macAddr[2], data.macAddr[3], data.macAddr[4], data.macAddr[5]);
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

void addPeer(const uint8_t * mac_addr, uint8_t chan){
  esp_now_peer_info_t peer;
  //ESP_ERROR_CHECK(esp_wifi_set_channel(chan ,WIFI_SECOND_CHAN_NONE));
  esp_now_del_peer(mac_addr);
  memset(&peer, 0, sizeof(esp_now_peer_info_t));
  peer.channel = chan;
  peer.ifidx = WIFI_IF_AP;
  peer.encrypt = false;
  memcpy(peer.peer_addr, mac_addr, sizeof(uint8_t[6]));
  
  if (esp_now_add_peer(&peer) != ESP_OK){
    log_d("Failed to add peer");
    return;
  }
  memcpy(m_u8BroadcastAddr, mac_addr, sizeof(uint8_t[6]));
}

void printMAC(const uint8_t * mac_addr){
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  log_d("mac_addr: %s", mac_addr);
}


PairingStatus autoPairing(){
String apMacAddr;
  switch(pairingStatus) {

    case PAIR_REQUEST:

      WiFi.mode(WIFI_AP);

      apMacAddr = WiFi.softAPmacAddress();
      apMacAddr.toLowerCase();

       //configure device AP mode
      configDeviceAP(apMacAddr);

      // This is the mac address of the Slave in AP Mode
      log_d("AP MAC: %s", apMacAddr.c_str());

      if ( 6 == sscanf(apMacAddr.c_str(), "%x:%x:%x:%x:%x:%x",  &espNowDataSent.macAddr[0], &espNowDataSent.macAddr[1], &espNowDataSent.macAddr[2], &espNowDataSent.macAddr[3], &espNowDataSent.macAddr[4], &espNowDataSent.macAddr[5] ) )
      {

      }

      log_d("Pairing request on channel %d",  channel);

      if (esp_now_init() != ESP_OK) {
        log_d("Error initializing ESP-NOW");
      }

      // set callback routines
      esp_now_register_send_cb(OnDataSent);
      esp_now_register_recv_cb(OnDataRecv);

      /* Set primary master key. */
      esp_now_set_pmk((uint8_t *)ESPNOW_PMK);
      ConfigBrodcastESPNow();
    
      // set pairing data to send to the server
      pairingData.msgType = PAIRING;
      pairingData.id = BOARD_ID;     
      pairingData.channel = channel;

      // add peer and send request
      addPeer(m_u8BroadcastAddr, channel);
      esp_now_send(m_u8BroadcastAddr, (uint8_t *) &pairingData, sizeof(pairingData));
      previousMillis = millis();

      

      pairingStatus = PAIR_REQUESTED;
    break;

    case PAIR_REQUESTED:
    // time out to allow receiving response from server
    currentMillis = millis();
    if(currentMillis - previousMillis > 250) {
      previousMillis = currentMillis;
      // time out expired,  try next channel
      channel ++;
      if (channel > MAX_CHANNEL){
         channel = 1;
      }   
      pairingStatus = PAIR_REQUEST;
    }
    break;

    case PAIR_PAIRED:
      // nothing to do here 
    break;
  }
  return pairingStatus;
}  
