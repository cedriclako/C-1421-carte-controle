#include "frame_sleep.h"
#include "frame_menu.h"
#include <WiFi.h>
#include "esp_log.h"

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  600        /* Time ESP32 will go to sleep (in seconds) */

extern ESPNOWRMT_SMsg espNowDataSent;
static uint8_t timeout_sleep = 0;

uint8_t curThermStatSleep = 0;
uint8_t lastThermStatSleep = 0;

void key_outside_cb(epdgui_args_vector_t &args) {
    Frame_Base *frame = EPDGUI_GetFrame("Frame_Menu");
    if (frame == NULL) {
        frame = new Frame_Menu();
        EPDGUI_AddFrame("Frame_Menu", frame);
    }
    EPDGUI_PushFrame(frame);
    EPDGUI_UpdateGlobalLastActiveTime();
    *((int *)(args[0])) = 0;
}

Frame_Sleep::Frame_Sleep(void) : Frame_Base(true) {
    _frame_name = "Frame_Sleep";

#if 1
    _sleep_comb_img = new M5EPD_Canvas(&M5.EPD);
    _sleep_comb_img->createCanvas(256, 256);
    _sleep_comb_img->fillCanvas(0);
   
   _below_area = new M5EPD_Canvas(&M5.EPD);
   _below_area->createCanvas(540, 64);
   _below_area->fillCanvas(0);

    if(GetThermostatValue())
        _sleep_comb_img->pushImage(0, 0, 256, 256, ImageResource_sleep_icon_flame_high_256x256);
    else
        _sleep_comb_img->pushImage(0, 0, 256, 256, ImageResource_sleep_icon_flame_low_256x256);

    _key_outside = new EPDGUI_Button("outside", 36, 860, 64, 64);

    _key_outside->CanvasNormal()->fillCanvas(0);
    _key_outside->CanvasNormal()->pushImage(0, 0, 64, 64, ImageResource_sleep_icon_outside_64x64);
    *(_key_outside->CanvasPressed()) =
        *(_key_outside->CanvasNormal());
    _key_outside->CanvasPressed()->ReverseColor();
    _key_outside->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void *)(&_is_run));
    _key_outside->Bind(EPDGUI_Button::EVENT_RELEASED, key_outside_cb);
    

    _frame_sleep = new M5EPD_Canvas(&M5.EPD);
    _room_value = new M5EPD_Canvas(&M5.EPD);

    M5.SHT30.UpdateData();
    tmp_room = M5.SHT30.GetTemperature();
    tmp_room-=TEMP_ADJ;

    log_d("tmp_room: %d", (uint16_t)((tmp_room) * 10));

    _room_value->createCanvas(175, 120);
    _room_value->fillCanvas(0);
    _room_value->setFreeFont(CB24);
    _room_value->setTextColor(15);
    _room_value->setTextSize(2);
    memset(buf_roomtemp, '\0', sizeof(buf_roomtemp));

    if (GetDegreesUnit() == DEGREES_CELSIUS) {
        sprintf(buf_roomtemp, "%.1f", tmp_room);
    }
    else{
        sprintf(buf_roomtemp, "%.1f", ((tmp_room) * 1.8) + 32);
    }
    _room_value->drawString(String(buf_roomtemp), 0, 0);

    _frame_sleep->createCanvas(540, 960);
    _frame_sleep->fillCanvas(0);
    _frame_sleep->setFreeFont(CB24);
    
    _frame_sleep->setTextColor(15);
    _frame_sleep->setTextSize(2);
    _frame_sleep->drawString("Room", 10, 45);

    _frame_sleep->setTextSize(1);
    _frame_sleep->setFreeFont(CB18);
    _frame_sleep->drawString("o", 455, 50);
    _frame_sleep->setTextSize(1);
    _frame_sleep->setFreeFont(CB18);

    if (GetDegreesUnit() == DEGREES_CELSIUS) {
        _frame_sleep->drawString("C", 475, 55);
    }
    else{
        _frame_sleep->drawString("F", 475, 55);
    }
    

    _frame_sleep->setFreeFont(CR24);
    _frame_sleep->setTextSize(1);
    memset(buf_setpoint, '\0', sizeof(buf_setpoint));

    if (GetDegreesUnit() == DEGREES_CELSIUS) {
        sprintf(buf_setpoint, "%.1f", (float)(GetSetpoint()/10.0));
    }
    else{
        sprintf(buf_setpoint, "%.1f", ((float)(GetSetpoint()/10.0) * 1.8) + 32);
    }
    
    if(GetThermostatMode() == AUTOMATIC)
    {
        _frame_sleep->drawString("Set point         " + String(buf_setpoint), 10, 195);
        _frame_sleep->setTextSize(1);
        _frame_sleep->setFreeFont(CR12);
        _frame_sleep->drawString("o", 370, 195);
        _frame_sleep->setTextSize(1);
        _frame_sleep->setFreeFont(CR12);
        if (GetDegreesUnit() == DEGREES_CELSIUS) {
            _frame_sleep->drawString("C", 385, 200);
        }
        else{
            _frame_sleep->drawString("F", 385, 200);
        }
    }

    _sbi_img = new M5EPD_Canvas(&M5.EPD);
    _sbi_img->createCanvas(48, 34);
    _sbi_img->pushImage(0, 0, 48, 34, ImageResource_sbi_icon_48x34);

    if (GetLanguage() == LANGUAGE_FR) {
        _canvas_title->drawString("Sommeil", 270, 34-10);
    }
    else {
        _canvas_title->drawString("Sleep", 270, 34-10);
    }
#endif

}

Frame_Sleep::~Frame_Sleep(void) {
    delete _sleep_comb_img;
    delete _frame_sleep;
    delete _room_value;
    delete _sbi_img;
    delete _key_outside;
    delete _below_area;
}

int Frame_Sleep::init(epdgui_args_vector_t &args) {
    _is_run = 1;
    M5.EPD.Clear();
   
    if(bEspNowInit)
    {
        //Set device in AP mode to begin with
        WiFi.mode(WIFI_AP);

        String apMacAddr = WiFi.softAPmacAddress();
        apMacAddr.toLowerCase();

        // configure device AP mode
        configDeviceAP(apMacAddr);
        // This is the mac address of the Slave in AP Mode
        log_d("AP MAC: %s", apMacAddr.c_str());

        if ( 6 == sscanf(apMacAddr.c_str(), "%x:%x:%x:%x:%x:%x",  &espNowDataSent.macAddr[0], &espNowDataSent.macAddr[1], &espNowDataSent.macAddr[2], &espNowDataSent.macAddr[3], &espNowDataSent.macAddr[4], &espNowDataSent.macAddr[5] ) )
        {

        }
        // Init ESPNow with a fallback logic
        InitESPNow();
        // Once ESPNow is successfully Init, we will register for recv CB to
        // get recv packer info.
        esp_now_register_recv_cb(OnDataRecv);
        esp_now_register_send_cb(OnDataSent);
        
        /* Set primary master key. */
        esp_now_set_pmk((uint8_t *)ESPNOW_PMK);
        ConfigBrodcastESPNow();
    }

    

    _time_update_room = 0; //millis();
    _frame_sleep->pushCanvas(0, 52 + 100, UPDATE_MODE_NONE);
    _room_value->pushCanvas(280, 100 + 100, UPDATE_MODE_NONE);
    _canvas_title->pushCanvas(0, 8, UPDATE_MODE_NONE);
    _sleep_comb_img->pushCanvas(142, 500, UPDATE_MODE_NONE);
   
    log_d("Init Frame_Sleep");

    _sbi_img->pushCanvas(10, 16, UPDATE_MODE_NONE);

    SetVoltageBattery(M5.getBatteryVoltage());
    
    _time_update_battery = millis();
    StatusBar(UPDATE_MODE_NONE, GetVoltageBattery());

    EPDGUI_AddObject(_key_outside);

    return 3;
}

int Frame_Sleep::run() {
    Frame_Base::run();

    if ((millis() - _time_update_room) > (TIME_UPDATE_ROOM_MS/12)) {
        
        M5.SHT30.UpdateData();
        tmp_room = M5.SHT30.GetTemperature();
        tmp_room-=TEMP_ADJ;

        StatusBar(UPDATE_MODE_DU4, GetVoltageBattery());

        _room_value->fillCanvas(0);
        _room_value->setFreeFont(CB24);
        _room_value->setTextColor(15);
        _room_value->setTextSize(2);
        memset(buf_roomtemp, '\0', sizeof(buf_roomtemp));
        if (GetDegreesUnit() == DEGREES_CELSIUS) {
            sprintf(buf_roomtemp, "%.1f", tmp_room);
        }
        else{
            sprintf(buf_roomtemp, "%.1f", ((tmp_room) * 1.8) + 32);
        }
        _room_value->drawString(String(buf_roomtemp), 0, 0);
        _room_value->pushCanvas(280, 100 + 100, UPDATE_MODE_DU4);

        if(GetThermostatMode() == AUTOMATIC)
        {
            if(GetSetpoint() > ((uint16_t)((tmp_room) * 10)))
            { 
                espNowDataSent.tStatRmt = true;
                curThermStatSleep = 1;
            }
            else
            {
                espNowDataSent.tStatRmt = false;
                curThermStatSleep = 2;
            }

            if(lastThermStatSleep != curThermStatSleep)
            {
                if(curThermStatSleep == 1)
                {
                    SetThermostatValue(1);
                    _sleep_comb_img->fillCanvas(0);
                    _sleep_comb_img->pushImage(0, 0, 256, 256, ImageResource_sleep_icon_flame_high_256x256);
                    _sleep_comb_img->pushCanvas(142, 500, UPDATE_MODE_GL16);
                }
                else if(curThermStatSleep == 2)
                {
                    SetThermostatValue(0);
                    _sleep_comb_img->fillCanvas(0);
                    _sleep_comb_img->pushImage(0, 0, 256, 256, ImageResource_sleep_icon_flame_low_256x256);
                    _sleep_comb_img->pushCanvas(142, 500, UPDATE_MODE_GL16);
                }
            }
            lastThermStatSleep = curThermStatSleep;
        }

        espNowDataSent.tStatRmt = GetThermostatValue();
        espNowDataSent.blowerSpeedRmt = GetBlowerSpeed();
        espNowDataSent.distribSpeedRmt = GetDistributionSpeed();
        espNowDataSent.boostStatRmt = GetBoostStat();

        log_d("sleep.tStatRmt: %d",espNowDataSent.tStatRmt);
        log_d("sleep.blowerSpeedRmt: %d",espNowDataSent.blowerSpeedRmt);
        log_d("sleep.distribSpeedRmt: %d",espNowDataSent.distribSpeedRmt);
        log_d("sleep.boostStatRmt: %d",espNowDataSent.boostStatRmt);

        sendDataToEspNow(espNowDataSent);

        _time_update_room = millis();
        
        timeout_sleep++;

        if(timeout_sleep > 5)
        {
            EPDGUI_Clear();
            _below_area->fillCanvas(0);
            _below_area->pushCanvas(36, 860, UPDATE_MODE_DU4);

            M5.EPD.Sleep();
            M5.disableEXTPower();
            M5.disableEPDPower();
            //M5.disableMainPower();
            log_d("esp_deep_sleep_start");
            
            gpio_deep_sleep_hold_en();
            gpio_hold_en((gpio_num_t) M5EPD_MAIN_PWR_PIN); 
            esp_sleep_enable_ext0_wakeup((gpio_num_t) M5EPD_KEY_PUSH_PIN, 0); //1 = High, 0 = Low
            esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
            esp_deep_sleep_start();
        }
    }

    return 1;
}