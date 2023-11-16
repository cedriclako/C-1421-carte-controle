#include "frame_setting_thermostatmode.h"

void sw_automatic_cb(epdgui_args_vector_t &args) {
    SetThermostatMode(AUTOMATIC);
    esp_restart();
}

void sw_manual_cb(epdgui_args_vector_t &args) {
    SetThermostatMode(MANUAL);
    esp_restart();
}

Frame_Setting_ThermostatMode::Frame_Setting_ThermostatMode(void) : Frame_Base(true) {
    _frame_name = "Frame_Setting_ThermostatMode";

#if 1
    sw_automatic = new EPDGUI_Switch(2, 4, 100, 532, 61);
    sw_manual = new EPDGUI_Switch(2, 4, 160, 532, 61);

    if (GetLanguage() == LANGUAGE_FR) {
        sw_automatic->SetLabel(0, "Automatique");
        sw_automatic->SetLabel(1, "Automatique");
        sw_automatic->Canvas(1)->ReverseColor();
        sw_automatic->Bind(1, &sw_automatic_cb);
        sw_manual->SetLabel(0, "Manuel");
        sw_manual->SetLabel(1, "Manuel");
        sw_manual->Canvas(1)->ReverseColor();
        sw_manual->Bind(1, &sw_manual_cb);
    }
    else
    {
        sw_automatic->SetLabel(0, "Automatic");
        sw_automatic->SetLabel(1, "Automatic");
        sw_automatic->Canvas(1)->ReverseColor();
        sw_automatic->Bind(1, &sw_automatic_cb);
        sw_manual->SetLabel(0, "Manual");
        sw_manual->SetLabel(1, "Manual");
        sw_manual->Canvas(1)->ReverseColor();
        sw_manual->Bind(1, &sw_manual_cb);
    }
    
    
    _sw_mutex_group = new EPDGUI_MutexSwitch();
    _sw_mutex_group->Add(sw_automatic);
    _sw_mutex_group->Add(sw_manual);
    
    if (GetLanguage() == LANGUAGE_FR) {
        exitbtn("Parametre");
        _canvas_title->drawString("Mode thermostat", 270, 34-10);
    } 
    else {
        exitbtn("Setting");
        _canvas_title->drawString("Thermostat mode", 270, 34-10);
    }

    if (GetThermostatMode() == AUTOMATIC) {
        sw_automatic->setState(1);
    }
    else
    {
        sw_manual->setState(1);
    }

    _key_exit->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void *)(&_is_run));
    _key_exit->Bind(EPDGUI_Button::EVENT_RELEASED, &Frame_Base::exit_cb);
#endif

}

Frame_Setting_ThermostatMode::~Frame_Setting_ThermostatMode(void) {
    delete sw_automatic;
    delete sw_manual;
    delete _sw_mutex_group;
}

int Frame_Setting_ThermostatMode::init(epdgui_args_vector_t &args) {
    _is_run = 1;
    M5.EPD.Clear();
    _shutdown_prompt_is_shown = false;
    _canvas_title->pushCanvas(0, 8, UPDATE_MODE_NONE);
    EPDGUI_AddObject(_sw_mutex_group);
    EPDGUI_AddObject(_key_exit);
    StatusBar(UPDATE_MODE_NONE, GetVoltageBattery());
    log_d("Init Frame_Setting_ThermostatMode");
    return 3;
}