#include "frame_livedata.h"
#include <WiFi.h>
#include "esp_log.h"
#include "espnowcomm.h"

#define POS_LX (15)
#define POS_RX (540 - 15)

void key_livedata_exit_cb(epdgui_args_vector_t &args) {
    EPDGUI_PopFrame(true);
    *((int *)(args[0])) = 0;
}

Frame_LiveData::Frame_LiveData(void) : Frame_Base(true) {
    _frame_name = "Frame_LiveData";

#if 1
    _canvas_base = new M5EPD_Canvas(&M5.EPD);
    _canvas_base->createCanvas(540, 796);
    _canvas_base->setTextDatum(CL_DATUM);
    
    if (GetLanguage() == LANGUAGE_FR) {
        exitbtn("Menu");
        _canvas_title->drawString("Donnees direct", 270, 34-10);
    }
    else {
        exitbtn("Menu");
        _canvas_title->drawString("Live data", 270, 34-10);
    }

    _key_exit->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void *)(&_is_run));
    _key_exit->Bind(EPDGUI_Button::EVENT_RELEASED, &Frame_Base::exit_cb);

#endif

}

Frame_LiveData::~Frame_LiveData(void) {
    delete _canvas_picture;
    delete _canvas_base;
}

void Frame_LiveData::err(String info) {
    _canvas_picture->fillCanvas(0);
    _canvas_picture->fillRect(254 - 150, 500 - 50, 300, 100, 15);
    _canvas_picture->drawString(info, 150, 55);
}

void Frame_LiveData::drawItem(String data, const char *str, int y) {
    
    _canvas_base->setTextDatum(CL_DATUM);
    _canvas_base->drawString(str, POS_LX, y);
    
    String myString = "";     // empty string
    myString.concat(data);
    _canvas_base->setTextDatum(CR_DATUM);
    _canvas_base->drawString(myString, POS_RX, y);
}

void Frame_LiveData::drawItem(m5epd_update_mode_t mode) {

    _canvas_base->fillCanvas(0);
    _canvas_base->setFreeFont(CR12);
    _canvas_base->setTextSize(1);
    _canvas_base->setTextColor(15);


   if (GetLanguage() == LANGUAGE_EN) {
        drawItem(String(dataDebug.time), "1.time", 30);
        drawItem(String(dataDebug.Tbaffle), "2.Tbaffle", 60);
        drawItem(String(dataDebug.Tavant), "3.Tavant", 90);
        drawItem(String(dataDebug.Plenum), "4.Plenum", 120);
        drawItem(String(dataDebug.State), "5.State", 150);
        drawItem(String(dataDebug.tStat), "6.tStat", 180);
        drawItem(String(dataDebug.dTbaffle), "7.dTbaffle", 210);
        drawItem(String(dataDebug.FanSpeed), "8.FanSpeed", 240);
        drawItem(String(dataDebug.Grille), "9.Grille", 270);
        drawItem(String(dataDebug.Prim), "10.Prim", 300);

        drawItem(String(dataDebug.Sec), "11.Sec", 330);
        drawItem(String(dataDebug.Tboard), "12.Tboard", 360);
        drawItem(String(dataDebug.Door), "13.Door", 390);
        drawItem(String(dataDebug.PartCH0ON), "14.PartCH0ON", 420);
        drawItem(String(dataDebug.PartCH1ON), "15.PartCH1ON", 450);
        drawItem(String(dataDebug.PartCH0OFF), "16.PartCH0OFF", 480);
        drawItem(String(dataDebug.PartCH1OFF), "17.PartCH1OFF", 510);
        drawItem(String(dataDebug.PartVar), "18.PartVar", 540);
        drawItem(String(dataDebug.PartSlope), "19.PartSlope", 570);
        drawItem(String(dataDebug.TPart), "20.TPart", 600);

        drawItem(String(dataDebug.PartCurr), "21.PartCurr", 630);
        drawItem(String(dataDebug.PartLuxON), "22.PartLuxON", 660);
        drawItem(String(dataDebug.PartLuxOFF), "23.PartLuxOFF", 690);
        drawItem(String(dataDebug.PartTime), "24.PartTime", 720);
        drawItem(String(dataDebug.dTavant), "25.dTavant", 750);
        drawItem(String(GetThermostatValue()), "26.tStatRmt", 780);

    } else {
        drawItem(String(dataDebug.time), "1.time", 30);
        drawItem(String(dataDebug.Tbaffle), "2.Tbaffle", 60);
        drawItem(String(dataDebug.Tavant), "3.Tavant", 90);
        drawItem(String(dataDebug.Plenum), "4.Plenum", 120);
        drawItem(String(dataDebug.State), "5.State", 150);
        drawItem(String(dataDebug.tStat), "6.tStat", 180);
        drawItem(String(dataDebug.dTbaffle), "7.dTbaffle", 210);
        drawItem(String(dataDebug.FanSpeed), "8.FanSpeed", 240);
        drawItem(String(dataDebug.Grille), "9.Grille", 270);
        drawItem(String(dataDebug.Prim), "10.Prim", 300);

        drawItem(String(dataDebug.Sec), "11.Sec", 330);
        drawItem(String(dataDebug.Tboard), "12.Tboard", 360);
        drawItem(String(dataDebug.Door), "13.Door", 390);
        drawItem(String(dataDebug.PartCH0ON), "14.PartCH0ON", 420);
        drawItem(String(dataDebug.PartCH1ON), "15.PartCH1ON", 450);
        drawItem(String(dataDebug.PartCH0OFF), "16.PartCH0OFF", 480);
        drawItem(String(dataDebug.PartCH1OFF), "17.PartCH1OFF", 510);
        drawItem(String(dataDebug.PartVar), "18.PartVar", 540);
        drawItem(String(dataDebug.PartSlope), "19.PartSlope", 570);
        drawItem(String(dataDebug.TPart), "20.TPart", 600);

        drawItem(String(dataDebug.PartCurr), "21.PartCurr", 630);
        drawItem(String(dataDebug.PartLuxON), "22.PartLuxON", 660);
        drawItem(String(dataDebug.PartLuxOFF), "23.PartLuxOFF", 690);
        drawItem(String(dataDebug.PartTime), "24.PartTime", 720);
        drawItem(String(dataDebug.dTavant), "25.dTavant", 750);
        drawItem(String(GetThermostatValue()), "26.tStatRmt", 780);
    }
    _canvas_base->pushCanvas(0, 94, mode);
}

int Frame_LiveData::init(epdgui_args_vector_t &args) {
    _is_run = 1;
    M5.EPD.Clear();
    _shutdown_prompt_is_shown = false;

    if(bEspNowInit)
    {
        //Set device in AP mode to begin with
        WiFi.mode(WIFI_AP);

        String apMacAddr = WiFi.softAPmacAddress();
        apMacAddr.toLowerCase();

        // configure device AP mode
        configDeviceAP(apMacAddr);
        // This is the mac address of the Slave in AP Mode
        log_d("AP MAC: %s", apMacAddr.c_str());

        if ( 6 == sscanf(apMacAddr.c_str(), "%x:%x:%x:%x:%x:%x",  &espNowDataSent.macAddr[0], &espNowDataSent.macAddr[1], &espNowDataSent.macAddr[2], &espNowDataSent.macAddr[3], &espNowDataSent.macAddr[4], &espNowDataSent.macAddr[5] ) )
        {

        }
        // Init ESPNow with a fallback logic
        InitESPNow();
        // Once ESPNow is successfully Init, we will register for recv CB to
        // get recv packer info.
        esp_now_register_recv_cb(OnDataRecv);
        esp_now_register_send_cb(OnDataSent);
        
        /* Set primary master key. */
        esp_now_set_pmk((uint8_t *)ESPNOW_PMK);
        ConfigBrodcastESPNow();
    }

    EPDGUI_AddObject(_key_exit); 
    _canvas_title->pushCanvas(0, 8, UPDATE_MODE_NONE);

    StatusBar(UPDATE_MODE_NONE, GetVoltageBattery());

    drawItem(UPDATE_MODE_NONE);
    _time_update_esp_now = 0;

    log_d("Init Frame_LiveData");
/*
    ScanForSlave();
    if (SlaveCnt > 0) { 
        manageSlave();
    } 
    */
    return 3;
}

int Frame_LiveData::run() {
    Frame_Base::run();

    
    if ((millis() - _time_update_esp_now) > (1000U)) {
        espNowDataSent.tStatRmt = GetThermostatValue();
        espNowDataSent.blowerSpeedRmt = GetBlowerSpeed();
        espNowDataSent.distribSpeedRmt = GetDistributionSpeed();
        espNowDataSent.boostStatRmt = GetBoostStat();
        if (autoPairing() == PAIR_PAIRED) {
            sendDataToEspNow(espNowDataSent);
        }
        _time_update_esp_now = millis();
        drawItem(UPDATE_MODE_DU4);
    }
    
    return 1;
}
