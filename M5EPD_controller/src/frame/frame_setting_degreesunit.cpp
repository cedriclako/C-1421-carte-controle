#include "frame_setting_degreesunit.h"

void sw_celsius_cb(epdgui_args_vector_t &args) {
    SetDegreesUnit(DEGREES_CELSIUS);
    esp_restart();
}

void sw_fahrenheit_cb(epdgui_args_vector_t &args) {
    SetDegreesUnit(DEGREES_FAHRENHEIT);
    esp_restart();
}

Frame_Setting_DegreesUnit::Frame_Setting_DegreesUnit(void) : Frame_Base(true) {
    _frame_name = "Frame_Setting_DegreesUnit";

#if 1
    sw_fahrenheit = new EPDGUI_Switch(2, 4, 100, 532, 61);
    sw_celsius = new EPDGUI_Switch(2, 4, 160, 532, 61);
    sw_fahrenheit->SetLabel(0, "Fahrenheit");
    sw_fahrenheit->SetLabel(1, "Fahrenheit");
    sw_fahrenheit->Canvas(1)->ReverseColor();
    sw_fahrenheit->Bind(1, &sw_fahrenheit_cb);
    sw_celsius->SetLabel(0, "Celsius");
    sw_celsius->SetLabel(1, "Celsius");
    sw_celsius->Canvas(1)->ReverseColor();
    sw_celsius->Bind(1, &sw_celsius_cb);

    _sw_mutex_group = new EPDGUI_MutexSwitch();
    _sw_mutex_group->Add(sw_fahrenheit);
    _sw_mutex_group->Add(sw_celsius);
   
    if (GetLanguage() == LANGUAGE_FR) {
        exitbtn("Parametre");
        _canvas_title->drawString("Unite temperature", 270, 34-10);
    } 
    else {
        exitbtn("Setting");
        _canvas_title->drawString("Temperature unit", 270, 34-10);
    }

    if (GetDegreesUnit() == DEGREES_CELSIUS) {
        sw_celsius->setState(1);
    }
    else
    {
        sw_fahrenheit->setState(1);
    }

    _key_exit->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void *)(&_is_run));
    _key_exit->Bind(EPDGUI_Button::EVENT_RELEASED, &Frame_Base::exit_cb);
#endif

}

Frame_Setting_DegreesUnit::~Frame_Setting_DegreesUnit(void) {
    delete sw_celsius;
    delete sw_fahrenheit;
    delete _sw_mutex_group;
}

int Frame_Setting_DegreesUnit::init(epdgui_args_vector_t &args) {
    _is_run = 1;
    M5.EPD.Clear();
    _shutdown_prompt_is_shown = false;
    _canvas_title->pushCanvas(0, 8, UPDATE_MODE_NONE);
    EPDGUI_AddObject(_sw_mutex_group);
    EPDGUI_AddObject(_key_exit);
    StatusBar(UPDATE_MODE_NONE, GetVoltageBattery());
    log_d("Init Frame_Setting_DegreesUnit");
    return 3;
}