#ifndef _FRAME_MENU_H_
#define _FRAME_MENU_H_

#include "frame_base.h"
#include "../epdgui/epdgui.h"

class Frame_Menu : public Frame_Base {
   public:
    Frame_Menu();
    ~Frame_Menu();
    int init(epdgui_args_vector_t &args);
    int run();

   private:
    EPDGUI_Button *_key[4];
    M5EPD_Canvas *_names_img_area;
    M5EPD_Canvas *_sbi_img;
};

#endif  //_FRAME_MENU_H_