#include <M5EPD.h>
#include "epdgui/epdgui.h"
#include "resources/binaryttf.h"
#include "Free_Fonts.h"
#include "resources/ImageResource.h"
#include "frame/frame.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"


#include <WiFi.h>
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_sleep.h"
#include "espnowcomm.h"


void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void init_nvs()
{
  // Initialize NVS
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      // NVS partition was truncated and needs to be erased
      // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK( err );
}


void setup()
{
  
  //delay(5);
  Serial.print("M5EPD initializing...");
  
  M5.begin(true, false, true, true);
  M5.TP.SetRotation(90);
  M5.EPD.SetRotation(90);
  M5.EPD.Clear(true);

  M5.SHT30.Begin();
  M5.SHT30.UpdateData();

  
  print_wakeup_reason();
  init_nvs();
  vTaskDelay( 10 / portTICK_PERIOD_MS );
  
  LoadSetting();
  
  vTaskDelay( 10 / portTICK_PERIOD_MS );
  
  
  Frame_Sleep *frame_sleep = new Frame_Sleep();
  EPDGUI_PushFrame(frame_sleep);


}

void loop()
{
    EPDGUI_MainLoop();    
}


