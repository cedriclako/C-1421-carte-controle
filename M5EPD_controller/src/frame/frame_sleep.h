#ifndef _FRAME_SLEEP_H_
#define _FRAME_SLEEP_H_

#include "frame_base.h"
#include "../epdgui/epdgui.h"

class Frame_Sleep : public Frame_Base {
   public:
    Frame_Sleep();
    ~Frame_Sleep();
    int init(epdgui_args_vector_t &args);
    int run();

   private:
    M5EPD_Canvas *_sleep_comb_img;
    M5EPD_Canvas *_room_value;
    M5EPD_Canvas *_below_area;
    M5EPD_Canvas *_frame_sleep;
    uint32_t _time_update_room;
    M5EPD_Canvas *_sbi_img;
    EPDGUI_Button *_key_outside;

};

#endif  //_FRAME_SLEEP_H_