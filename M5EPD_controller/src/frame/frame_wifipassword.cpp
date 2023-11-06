#include "frame_wifipassword.h"

void key_passwordclear_cb(epdgui_args_vector_t &args) {
    ((EPDGUI_Textbox *)(args[0]))->SetText("");
}

Frame_WifiPassword::Frame_WifiPassword(bool isHorizontal) : Frame_Base() {
    _frame_name      = "Frame_WifiPassword";

#if 1
    uint8_t language = GetLanguage();
    if (isHorizontal) {
        inputbox = new EPDGUI_Textbox(84, 25, 712, 250);
        key_textclear = new EPDGUI_Button("CLR", 804, 25, 72, 120);
    } else {
        const uint16_t kKeyBaseY = 176;
        inputbox                 = new EPDGUI_Textbox(4, 100, 532, 60);
        key_textclear = new EPDGUI_Button("CLR", 4, kKeyBaseY, 260, 52);
    }

    inputbox->SetTextMargin(8, 15, 8, 8);
    inputbox->SetState(EPDGUI_Textbox::EVENT_PRESSED);

    keyboard = new EPDGUI_Keyboard(
        isHorizontal, EPDGUI_Keyboard::STYLE_INPUTMODE_NEEDCONFIRM);

    key_textclear->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void *)inputbox);
    key_textclear->Bind(EPDGUI_Button::EVENT_RELEASED, key_passwordclear_cb);

    _key_exit->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void *)(&_is_run));
    _key_exit->Bind(EPDGUI_Button::EVENT_RELEASED, &Frame_Base::exit_cb);
#endif

}

Frame_WifiPassword::~Frame_WifiPassword() {
    delete inputbox;
    delete keyboard;
    delete key_textclear;
}

int Frame_WifiPassword::init(epdgui_args_vector_t &args) {
    _is_run = 1;
    M5.EPD.Clear();
    _shutdown_prompt_is_shown = false;

#if 1
    uint8_t language = GetLanguage();

    if (language == LANGUAGE_FR) {
        exitbtn("Reseau");
        _canvas_title->fillCanvas(0);
        _canvas_title->drawFastHLine(0, 62, 540, 15);
        _canvas_title->drawFastHLine(0, 61, 540, 15);
        _canvas_title->drawFastHLine(0, 60, 540, 15);
        _canvas_title->drawString("Mot de passe", 270, 34);
    } else {
        exitbtn("Wlan");
        _canvas_title->fillCanvas(0);
        _canvas_title->drawFastHLine(0, 62, 540, 15);
        _canvas_title->drawFastHLine(0, 61, 540, 15);
        _canvas_title->drawFastHLine(0, 60, 540, 15);
        _canvas_title->drawString("Password", 270, 34);
    }
#endif


    _canvas_title->pushCanvas(0, 8, UPDATE_MODE_NONE);
    EPDGUI_AddObject(inputbox);
    EPDGUI_AddObject(keyboard);
    EPDGUI_AddObject(_key_exit);
    EPDGUI_AddObject(key_textclear);
    StatusBar(UPDATE_MODE_NONE, GetVoltageBattery());
    log_d("Init Frame_WifiPassword");
    return 6;
}

int Frame_WifiPassword::run(void) {
    Frame_Base::run();
    String data = keyboard->getData();
    if (data.indexOf("\n") >= 0) {
        String *pswd = new String(inputbox->GetText());
        EPDGUI_AddFrameArg("Frame_WifiScan", 0, pswd);
        inputbox->SetText("");
        EPDGUI_PopFrame();
        _is_run = 0;
        return 0;
    }
    inputbox->AddText(data);
    return 1;
}
