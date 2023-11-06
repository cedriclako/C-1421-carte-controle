#include "frame_base.h"
#include "frame_sleep.h"
#include "../epdgui/epdgui.h"
#include <WiFi.h>

Frame_Base::Frame_Base(bool _has_title) {
    
    M5.EPD.Clear();

    if (_has_title) {
        _canvas_title = new M5EPD_Canvas(&M5.EPD);
        _canvas_title->createCanvas(540, 62);
        _canvas_title->fillCanvas(0);
        _canvas_title->drawFastHLine(0, 62, 540, 15);
        _canvas_title->drawFastHLine(0, 61, 540, 15);
        _canvas_title->drawFastHLine(0, 60, 540, 15);
        _canvas_title->setFreeFont(CB12);
        _canvas_title->setTextSize(1);
        _canvas_title->setTextDatum(CC_DATUM);
    }
    
    _bar = new M5EPD_Canvas(&M5.EPD);
    _bar->createCanvas(200, 32);

    _frame_id   = 0;
    _frame_name = "Frame_Base";

    _time_update_battery             = millis();

    EPDGUI_UpdateGlobalLastActiveTime();
    SetVoltageBattery(M5.getBatteryVoltage());
    StatusBar(UPDATE_MODE_NONE, GetVoltageBattery());
    log_d("Init Frame_Base");
}

Frame_Base::~Frame_Base() {
    if (_key_exit != NULL) delete _key_exit;
    if (_canvas_title != NULL) delete _canvas_title;
    delete _bar;
}

void Frame_Base::exitbtn(String title, uint16_t width) {
    _key_exit = new EPDGUI_Button(8, 12, width, 48);
    _key_exit->CanvasNormal()->fillCanvas(0);
    _key_exit->CanvasNormal()->setFreeFont(CB12);
    _key_exit->CanvasNormal()->setTextSize(1);
    _key_exit->CanvasNormal()->setTextDatum(CL_DATUM);
    _key_exit->CanvasNormal()->setTextColor(15);
    _key_exit->CanvasNormal()->drawString(title, 47 + 13 - 6, 28-6);
    _key_exit->CanvasNormal()->pushImage(15, 8, 32, 32, ImageResource_item_icon_arrow_l_32x32);
    *(_key_exit->CanvasPressed()) = *(_key_exit->CanvasNormal());
    _key_exit->CanvasPressed()->ReverseColor();
}


void Frame_Base::CheckAutoPowerSave() {
    unsigned long now = millis();

   if (((millis() - g_last_active_time_millis) > (TIME_BEFORE_SHUTDOWN_PROMPT_MS/2)) && (_shutdown_prompt_is_shown == false)) {
        log_d("Show shutdown prompt");
        SaveSetting();
        _shutdown_prompt_is_shown = true;
        //g_last_active_time_millis =  millis();

        if(_shutdown_prompt_is_shown == true) {
            log_d("_shutdown_prompt_is_shown is true ..............");
            Frame_Base *frame = EPDGUI_GetFrame("Frame_Sleep");
            if (frame == NULL) {
                frame = new Frame_Sleep();
                EPDGUI_AddFrame("Frame_Sleep", frame);
            }
            WiFi.mode(WIFI_OFF);
            _is_run = 0;
            EPDGUI_PushFrame(frame);
        }

    } 
    
}

void Frame_Base::StatusBar(m5epd_update_mode_t mode, uint32_t vol) {
    char buf[20];
    /*
    _bar->fillCanvas(0);
    _bar->drawFastHLine(0, 43, 540, 15);
    _bar->setTextDatum(CL_DATUM);
    _bar->drawString("M5Paper", 10, 27);
    */
    _bar->fillCanvas(0);
    _bar->setFreeFont(CB12);
    _bar->setTextColor(15);
    _bar->setTextSize(1);
    //_bar->drawString("SBI", 10, 8);

    // Battery
    //_bar->setTextDatum(CR_DATUM);
    _bar->pushImage(488 - 400 + 5, (8-8), 32, 32, ImageResource_status_bar_battery_32x32);
    

    if (vol < 3300) {
        vol = 3300;
    } else if (vol > 4350) {
        vol = 4350;
    }
    float battery = (float)(vol - 3300) / (float)(4350 - 3300);
    if (battery <= 0.01) {
        battery = 0.01;
    }
    if (battery > 1) {
        battery = 1;
    }
    uint8_t px = battery * 25;
    memset(buf, '\0', sizeof(buf));
    sprintf(buf, "%d%%", (int)(battery * 100));
    // _bar->drawString(buf, 498 - 10, 27);
    _bar->fillRect(488 - 400 + 3 + 5, (8-8) + 10, px, 12, 15);
    // _bar->pushImage(498, 8, 32, 32, 2,
    // ImageResource_status_bar_battery_charging_32x32);

    // Time
    rtc_time_t time_struct;
    rtc_date_t date_struct;
    M5.RTC.getTime(&time_struct);
    M5.RTC.getDate(&date_struct);
    //sprintf(buf, "%2d:%02d", time_struct.hour, time_struct.min);
    //_bar->setTextDatum(CC_DATUM);
    if (battery >= 1) {
        _bar->drawString(buf, 450 - 412 , 8 - 2);
    }
    else
    {
        _bar->drawString(buf, 450 - 402 , 8 - 2);
    }
    
    _bar->pushCanvas(400, 20, mode);

    log_d("StatusBar entry");
}

int Frame_Base::run(void) {
    if (ENABLE_AUTO_POWER_SAVE) {
        CheckAutoPowerSave();
        //log_d("etat shutdown: %d", _shutdown_prompt_is_shown);
    }
    if ((millis() - _time_update_battery) > (TIME_UPDATE_BATTERY_VOLTAGE_MS/3)) {  // 20 secondes
        SetVoltageBattery(M5.getBatteryVoltage());
        StatusBar(UPDATE_MODE_DU4, GetVoltageBattery());
        _time_update_battery = millis();   
    }
    
    return _is_run;
}

void Frame_Base::exit(void) {
}

void Frame_Base::exit_cb(epdgui_args_vector_t &args) {
    EPDGUI_PopFrame();
    LoadSetting();
    //Set device in OFF mode to end
    WiFi.mode(WIFI_OFF);
    *((int *)(args[0])) = 0;
}


