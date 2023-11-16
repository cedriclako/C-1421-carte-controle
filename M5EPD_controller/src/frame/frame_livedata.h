#ifndef _FRAME_PICTUREVIEWER_H_
#define _FRAME_PICTUREVIEWER_H_

#include "frame_base.h"
#include "../epdgui/epdgui.h"

class Frame_LiveData : public Frame_Base {
   public:
    Frame_LiveData();
    ~Frame_LiveData();
    int init(epdgui_args_vector_t &args);
    int run();
    void err(String info);
    void drawItem(m5epd_update_mode_t mode);
    void drawItem(String data, const char *str, int y);

   private:
    M5EPD_Canvas *_canvas_picture;
    M5EPD_Canvas *_canvas_base;
    uint32_t _time_update_esp_now;
};

#endif  //_FRAME_PICTUREVIEWER_H_