#ifndef _FRAME_MAIN_H_
#define _FRAME_MAIN_H_

#include "frame_base.h"
#include "../epdgui/epdgui.h"

class Frame_Home : public Frame_Base {
   public:
    Frame_Home();
    ~Frame_Home();
    int run();
    int init(epdgui_args_vector_t &args);
    void update_temp_room(float tmp);
    void update_setpoint(char arr_setpoint[], uint16_t val_setpoint);

   private:
    EPDGUI_Button *_key[5];
    M5EPD_Canvas *_status_blower_area;
    M5EPD_Canvas *_status_distrib_area;
    M5EPD_Canvas *_status_boost_area;
    M5EPD_Canvas *_frame_home;
    M5EPD_Canvas *_room_area;
    M5EPD_Canvas *_setpoint_area;
    M5EPD_Canvas *_state_img;
    M5EPD_Canvas *_smoke_img;
    uint32_t _next_update_time;
    uint32_t _time;
    uint32_t _time_update_room;
    uint32_t _time_update_esp_now;
    uint16_t _setpoint_value;
    EPDGUI_MutexSwitch* _sw_mutex_group;
    EPDGUI_Switch* _sw_high;
    EPDGUI_Switch* _sw_low;

   
};

#endif  //_FRAME_MAIN_H_