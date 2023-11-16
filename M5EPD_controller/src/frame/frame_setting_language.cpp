#include "frame_setting_language.h"

void sw_en_cb(epdgui_args_vector_t &args) {
    SetLanguage(LANGUAGE_EN);
    log_d("Select_Setting_Language: EN");
    esp_restart();
}

void sw_fr_cb(epdgui_args_vector_t &args) {
    SetLanguage(LANGUAGE_FR);
    log_d("Select_Setting_Language: FR");
    esp_restart();
}

Frame_Setting_Language::Frame_Setting_Language(void) : Frame_Base(true) {
    _frame_name = "Frame_Setting_Language";

#if 1
    _sw_en = new EPDGUI_Switch(2, 4, 100, 532, 61);
    _sw_fr = new EPDGUI_Switch(2, 4, 160, 532, 61);

    _sw_en->SetLabel(0, "English");
    _sw_en->SetLabel(1, "English");
    _sw_en->Canvas(1)->ReverseColor();
    _sw_en->Bind(1, &sw_en_cb);

    _sw_fr->SetLabel(0, "French");
    _sw_fr->SetLabel(1, "French");
    _sw_fr->Canvas(1)->ReverseColor();
    _sw_fr->Bind(1, &sw_fr_cb);

    _sw_mutex_group = new EPDGUI_MutexSwitch();
    _sw_mutex_group->Add(_sw_en);
    _sw_mutex_group->Add(_sw_fr);
    
    if (GetLanguage() == LANGUAGE_FR) {
        exitbtn("Parametre");
        _canvas_title->drawString("Langue", 270, 34-10);
        _sw_fr->setState(1);
    } 
    else {
        exitbtn("Setting");
        _canvas_title->drawString("Language", 270, 34-10);
        _sw_en->setState(1);
    }

    _key_exit->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void *)(&_is_run));
    _key_exit->Bind(EPDGUI_Button::EVENT_RELEASED, &Frame_Base::exit_cb);
#endif

}

Frame_Setting_Language::~Frame_Setting_Language(void) {
    delete _sw_en;
    delete _sw_fr;
    delete _sw_mutex_group;
}

int Frame_Setting_Language::init(epdgui_args_vector_t &args) {
    _is_run = 1;
    M5.EPD.Clear();
    _shutdown_prompt_is_shown = false;
    _canvas_title->pushCanvas(0, 8, UPDATE_MODE_NONE);
    EPDGUI_AddObject(_sw_mutex_group);
    EPDGUI_AddObject(_key_exit);
    StatusBar(UPDATE_MODE_NONE, GetVoltageBattery());
    log_d("Init Frame_Setting_Language");
    return 3;
}