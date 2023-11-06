#ifndef _FRAME_BASE_H_
#define _FRAME_BASE_H_

#include <M5EPD.h>
#include "../epdgui/epdgui_button.h"
#include "../epdgui/epdgui_textbox.h"
#include "../global_setting.h"
#include "../resources/ImageResource.h"
#include "Free_Fonts.h"
#include "espnowcomm.h"

#define TEMP_ADJ 4

class Frame_Base {
   public:
    Frame_Base(bool _has_title = true);
    void exitbtn(String title, uint16_t width = 150);
    virtual ~Frame_Base();
    virtual int run();
    virtual void exit();
    virtual int init(epdgui_args_vector_t &args) = 0;
    virtual void StatusBar(m5epd_update_mode_t mode, uint32_t vol);
    String GetFrameName() {
        return _frame_name;
    }
    int isRun() {
        return _is_run;
    }
    void SetFrameID(uint32_t id) {
        _frame_id = id;
    }
    uint32_t GetFrameID() {
        return _frame_id;
    }

   protected:
    static void exit_cb(epdgui_args_vector_t &args);
    void UpdateLastActiveTime();
    String _frame_name;
    int _is_run                  = 1;
    M5EPD_Canvas *_canvas_title;
    M5EPD_Canvas *_bar;
    EPDGUI_Button *_key_exit;
    uint32_t _frame_id;
    bool _shutdown_prompt_is_shown = false;
    char buf_roomtemp[6];
    char buf_setpoint[6];
    float tmp_room;
    bool bEspNowInit            = true;
    uint32_t _time_update_battery;
    
    
   private:
    void CheckAutoPowerSave();
    
    
    
};

#endif