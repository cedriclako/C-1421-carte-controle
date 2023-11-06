#include "frame_setting.h"
#include "frame_setting_language.h"
#include "frame_setting_degreesunit.h"
#include "frame_setting_thermostatmode.h"
#include "frame_sleep.h"
#include "WiFi.h"

#define KEY_W 92
#define KEY_H 92
const uint16_t kTimeZoneY = 520;

void key_restart_cb(epdgui_args_vector_t &args) {
    //M5.EPD.WriteFullGram4bpp(GetWallpaper());
    M5.EPD.UpdateFull(UPDATE_MODE_GC16);
    SaveSetting();
    esp_restart();
}

void key_language_cb(epdgui_args_vector_t &args) {
    Frame_Base *frame = EPDGUI_GetFrame("Frame_Setting_Language");
    if (frame == NULL) {
        frame = new Frame_Setting_Language();
        EPDGUI_AddFrame("Frame_Setting_Language", frame);
    }
    EPDGUI_PushFrame(frame);
    *((int *)(args[0])) = 0;
}

void key_degrees_unit(epdgui_args_vector_t &args) {
   Frame_Base *frame = EPDGUI_GetFrame("Frame_Setting_DegreesUnit");
    if (frame == NULL) {
        frame = new Frame_Setting_DegreesUnit();
        EPDGUI_AddFrame("Frame_Setting_DegreesUnit", frame);
    }
    EPDGUI_PushFrame(frame);
    *((int *)(args[0])) = 0;
}

void key_thermostat_mode(epdgui_args_vector_t &args) {
   Frame_Base *frame = EPDGUI_GetFrame("Frame_Setting_ThermostatMode");
    if (frame == NULL) {
        frame = new Frame_Setting_ThermostatMode();
        EPDGUI_AddFrame("Frame_Setting_ThermostatMode", frame);
    }
    EPDGUI_PushFrame(frame);
    *((int *)(args[0])) = 0;
}

Frame_Setting::Frame_Setting(void) : Frame_Base(true) {
    _frame_name = "Frame_Setting";
    //_frame_id   = 5;
#if 1
    _mac_address_canvas = new M5EPD_Canvas(&M5.EPD);
    _mac_address_canvas->createCanvas(540, 60);
    _mac_address_canvas->fillCanvas(0);
    _mac_address_canvas->setFreeFont(CR12);
    _mac_address_canvas->setTextSize(1);
    _mac_address_canvas->setTextColor(15);
    _mac_address_canvas->setTextDatum(CL_DATUM);

    _key_language    = new EPDGUI_Button(4, 100, 532, 61);
    _key_degrees_unit     = new EPDGUI_Button(4, 160, 532, 61);
    _key_thermostat_mode     = new EPDGUI_Button(4, 220, 532, 61);
    _key_restart     = new EPDGUI_Button(4, 280, 532, 61);

    WiFi.mode(WIFI_AP);
    String apMacAddr = WiFi.softAPmacAddress();
    apMacAddr.toLowerCase();

    if (GetLanguage() == LANGUAGE_FR) {
        _key_language->setBMPButton("  Langue", "\u25B6",
                                    ImageResource_item_icon_language_32x32);
        _key_degrees_unit->setBMPButton("  Unite temperature", "",
                                   ImageResource_item_icon_degrees_unit_32x32);
        _key_thermostat_mode->setBMPButton("  Mode thermostat", "",
                                   ImageResource_item_icon_thermostat_mode_32x32);
        _key_restart->setBMPButton("  Redemarrage", "",
                                   ImageResource_item_icon_restart_32x32);
        

        exitbtn("Menu");
        _canvas_title->drawString("Parametre", 270, 34-10);
        
        _mac_address_canvas->drawString("Adresse mac: " + apMacAddr, 15, 340);
    } 
    else {
        _key_language->setBMPButton("  Language", "\u25B6",
                                    ImageResource_item_icon_language_32x32);
        _key_degrees_unit->setBMPButton("  Temperature unit", "",
                                   ImageResource_item_icon_degrees_unit_32x32);
        _key_thermostat_mode->setBMPButton("  Thermostat mode", "",
                                   ImageResource_item_icon_thermostat_mode_32x32);
        _key_restart->setBMPButton("  Restart", "",
                                   ImageResource_item_icon_restart_32x32);
    
        exitbtn("Menu");
        _canvas_title->drawString("Setting", 270, 34-10);
        _mac_address_canvas->drawString("Mac address: " + apMacAddr, 15, 340);
    }

    

    _key_language->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void *)(&_is_run));
    _key_language->Bind(EPDGUI_Button::EVENT_RELEASED, &key_language_cb);

    _key_restart->Bind(EPDGUI_Button::EVENT_RELEASED, &key_restart_cb);

    _key_degrees_unit->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void *)(&_is_run));
    _key_degrees_unit->Bind(EPDGUI_Button::EVENT_RELEASED, &key_degrees_unit);

    _key_thermostat_mode->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void *)(&_is_run));
    _key_thermostat_mode->Bind(EPDGUI_Button::EVENT_RELEASED, &key_thermostat_mode);

    _key_exit->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void *)(&_is_run));
    _key_exit->Bind(EPDGUI_Button::EVENT_RELEASED, &Frame_Base::exit_cb);
    
#endif

}

Frame_Setting::~Frame_Setting(void) {
    delete _key_language;
    delete _key_restart;
    delete _key_degrees_unit;
}

int Frame_Setting::init(epdgui_args_vector_t &args) {
    _is_run = 1;
    M5.EPD.Clear();
    _shutdown_prompt_is_shown = false;

    _canvas_title->pushCanvas(0, 8, UPDATE_MODE_NONE);
    _mac_address_canvas->pushCanvas(0, 340, UPDATE_MODE_NONE);
    
    EPDGUI_AddObject(_key_language);
    EPDGUI_AddObject(_key_restart);
    EPDGUI_AddObject(_key_degrees_unit);
    EPDGUI_AddObject(_key_thermostat_mode);
    EPDGUI_AddObject(_key_exit);
    StatusBar(UPDATE_MODE_NONE, GetVoltageBattery());
    log_d("Init Frame_Setting");
    
    return 3;
}

int Frame_Setting::run() {
    Frame_Base::run();
    return 1;
}
