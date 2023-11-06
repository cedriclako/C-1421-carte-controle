#ifndef _FRAME_THERMOSTAT_MODE_H_
#define _FRAME_THERMOSTAT_MODE_H_

#include "frame_base.h"
#include "../epdgui/epdgui.h"

class Frame_Setting_ThermostatMode : public Frame_Base {
   public:
    Frame_Setting_ThermostatMode();
    ~Frame_Setting_ThermostatMode();
    int init(epdgui_args_vector_t& args);

   private:
    EPDGUI_Switch* sw_automatic;
    EPDGUI_Switch* sw_manual;
    EPDGUI_MutexSwitch* _sw_mutex_group;
};

#endif  //_FRAME_THERMOSTAT_MODE_H_