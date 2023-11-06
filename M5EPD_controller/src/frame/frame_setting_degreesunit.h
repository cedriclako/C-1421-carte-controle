#ifndef _FRAME_DEGREES_UNIT_H_
#define _FRAME_DEGREES_UNIT_H_

#include "frame_base.h"
#include "../epdgui/epdgui.h"

class Frame_Setting_DegreesUnit : public Frame_Base {
   public:
    Frame_Setting_DegreesUnit();
    ~Frame_Setting_DegreesUnit();
    int init(epdgui_args_vector_t& args);

   private:
    EPDGUI_Switch* sw_celsius;
    EPDGUI_Switch* sw_fahrenheit;
    EPDGUI_MutexSwitch* _sw_mutex_group;
};

#endif  //_FRAME_DEGREES_UNIT_H_