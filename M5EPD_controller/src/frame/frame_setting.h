#ifndef _FRAME_SETTING_H_
#define _FRAME_SETTING_H_

#include "frame_base.h"
#include "../epdgui/epdgui.h"

class Frame_Setting : public Frame_Base {
   public:
    Frame_Setting();
    ~Frame_Setting();
    int init(epdgui_args_vector_t &args);
    int run();

   private:
    EPDGUI_Button *_key_language;
    EPDGUI_Button *_key_restart;
    EPDGUI_Button *_key_degrees_unit;
    EPDGUI_Button *_key_thermostat_mode;

    M5EPD_Canvas *_mac_address_canvas;
};

#endif  //_FRAME_SETTING_H_