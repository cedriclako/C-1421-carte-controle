#include "frame_menu.h"
#include "frame_setting.h"
#include "frame_wifiscan.h"
#include "frame_livedata.h"
#include "frame_home.h"

enum {
    kKeyHome = 0,
    kKeySetting,
    kKeyLiveData,
    //kKeyWlan,
    kKeyIdle
};

#define KEY_W 132
#define KEY_H 132

void key_home_cb(epdgui_args_vector_t &args) {
    Frame_Base *frame = EPDGUI_GetFrame("Frame_Home");
    if (frame == NULL) {
        frame = new Frame_Home();
        EPDGUI_AddFrame("Frame_Home", frame);
    }
    EPDGUI_PushFrame(frame);
    *((int *)(args[0])) = 0;
}

void key_setting_cb(epdgui_args_vector_t &args) {
    Frame_Base *frame = EPDGUI_GetFrame("Frame_Setting");
    if (frame == NULL) {
        frame = new Frame_Setting();
        EPDGUI_AddFrame("Frame_Setting", frame);
    }
    EPDGUI_PushFrame(frame);
    *((int *)(args[0])) = 0;
}

void key_livedata_cb(epdgui_args_vector_t &args) {
    Frame_Base *frame = EPDGUI_GetFrame("Frame_LiveData");
    if (frame == NULL) {
        frame = new Frame_LiveData();
        EPDGUI_AddFrame("Frame_LiveData", frame);
    }
    EPDGUI_PushFrame(frame);
    *((int *)(args[0])) = 0;
}

void key_wlan_cb(epdgui_args_vector_t &args) {
    Frame_Base *frame = EPDGUI_GetFrame("Frame_WifiScan");
    if (frame == NULL) {
        frame = new Frame_WifiScan();
        EPDGUI_AddFrame("Frame_WifiScan", frame);
    }
    EPDGUI_PushFrame(frame);
    *((int *)(args[0])) = 0;
}

Frame_Menu::Frame_Menu(void) : Frame_Base(true) {
    _frame_name = "Frame_Menu";
    _frame_id   = 1;

#if 1
    _sbi_img = new M5EPD_Canvas(&M5.EPD);
    _sbi_img->createCanvas(48, 34);
    _sbi_img->pushImage(0, 0, 48, 34, ImageResource_sbi_icon_48x34);

    _names_img_area = new M5EPD_Canvas(&M5.EPD);
    _names_img_area->createCanvas(540, 500);
    

    for (int i = 0; i < 2; i++) {
        _key[i] = new EPDGUI_Button("button", 92 + i * (92 + 132), (188*1), KEY_W, KEY_H);
        
    }

    for (int i = 0; i < (2-1); i++) {
        _key[i + 2] = new EPDGUI_Button("button", 92 + i * (92 + 132), (188*2) + 132 , KEY_W, KEY_H);
    }

        

    _key[kKeyHome]->CanvasNormal()->fillCanvas(0);
    _key[kKeyHome]->CanvasNormal()->pushImage(0, 0, KEY_W, KEY_H, ImageResource_menu_icon_home_132x132);
    *(_key[kKeyHome]->CanvasPressed()) =
        *(_key[kKeyHome]->CanvasNormal());
    _key[kKeyHome]->CanvasPressed()->ReverseColor();
    _key[kKeyHome]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void *)(&_is_run));
    _key[kKeyHome]->Bind(EPDGUI_Button::EVENT_RELEASED, key_home_cb);
    
    _key[kKeySetting]->CanvasNormal()->fillCanvas(0);
    _key[kKeySetting]->CanvasNormal()->pushImage(0, 0, KEY_W, KEY_H, ImageResource_menu_icon_setting_132x132);
    *(_key[kKeySetting]->CanvasPressed()) =
        *(_key[kKeySetting]->CanvasNormal());
    _key[kKeySetting]->CanvasPressed()->ReverseColor();
    _key[kKeySetting]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void *)(&_is_run));
    _key[kKeySetting]->Bind(EPDGUI_Button::EVENT_RELEASED, key_setting_cb);

    _key[kKeyLiveData]->CanvasNormal()->fillCanvas(0);
    _key[kKeyLiveData]->CanvasNormal()->pushImage(0, 0, KEY_W, KEY_H, ImageResource_menu_icon_livedata_132x132);
    *(_key[kKeyLiveData]->CanvasPressed()) =
        *(_key[kKeyLiveData]->CanvasNormal());
    _key[kKeyLiveData]->CanvasPressed()->ReverseColor();
    _key[kKeyLiveData]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void *)(&_is_run));
    _key[kKeyLiveData]->Bind(EPDGUI_Button::EVENT_RELEASED, key_livedata_cb);

#if 0
    _key[kKeyWlan]->CanvasNormal()->fillCanvas(0);
    _key[kKeyWlan]->CanvasNormal()->pushImage(0, 0, KEY_W, KEY_H, ImageResource_menu_icon_wlan_132x132);
    *(_key[kKeyWlan]->CanvasPressed()) =
        *(_key[kKeyWlan]->CanvasNormal());
    _key[kKeyWlan]->CanvasPressed()->ReverseColor();
    _key[kKeyWlan]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void *)(&_is_run));
    _key[kKeyWlan]->Bind(EPDGUI_Button::EVENT_RELEASED, key_wlan_cb);
#endif
   
    _names_img_area->fillCanvas(0);
    _names_img_area->setFreeFont(CB12);
    _names_img_area->setTextDatum(CC_DATUM);
    _names_img_area->setTextSize(1);

    if (GetLanguage() == LANGUAGE_FR) {
         _names_img_area->drawString("Piece", 46+24, 168);
        _names_img_area->drawString("Parametre", 46+18 + 132+92, 168);
        _names_img_area->drawString("Donnees Direct", 46+18, 168 + 188 + 132);
        //_names_img_area->drawString("Reseau", 46+24 + 132+92, 168 + 188 + 132);
        _canvas_title->drawString("Menu", 270, 34-10);
    }
    else {
         _names_img_area->drawString("Home", 46+24, 168);
        _names_img_area->drawString("Setting", 46+18 + 132+92, 168);
        _names_img_area->drawString("LiveData", 46+18, 168 + 188 + 132);
        //_names_img_area->drawString("Wlan", 46+24 + 132+92, 168 + 188 + 132);
        _canvas_title->drawString("Menu", 270, 34-10);
    }
#endif

}

Frame_Menu::~Frame_Menu(void) {
    for (int i = 0; i < 4; i++) {
        delete _key[i];
    }

    delete _names_img_area;
    delete _sbi_img;
}

int Frame_Menu::init(epdgui_args_vector_t &args) {
    _is_run = 1;
    M5.EPD.Clear();
    _shutdown_prompt_is_shown = false;

    for (int i = 0; i < kKeyIdle; i++) {
        EPDGUI_AddObject(_key[i]);
    }

    _names_img_area->pushCanvas(92, 164, UPDATE_MODE_NONE);
    _canvas_title->pushCanvas(0, 8, UPDATE_MODE_NONE);
    _sbi_img->pushCanvas(10, 16, UPDATE_MODE_NONE);

    StatusBar(UPDATE_MODE_NONE, GetVoltageBattery());

    log_d("Init Frame_Menu");

    return 3;
}


int Frame_Menu::run() {
    Frame_Base::run();

    return 1;
}
