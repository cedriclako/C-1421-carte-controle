#include "frame_home.h"
#include <WiFi.h>
#include "esp_log.h"
#include "espnowcomm.h"

enum
{
    KKeySetPointUp = 0,
    KKeySetPointDown,
    KKeyBlower,
    KKeyDistribution,
    KKeyBoost,
    KKeyNone
};

String blower_str = "off";
uint8_t idx_blower = 0;
bool onClickBlower = false;

String distribution_str = "off";
uint8_t idx_distribution = 0;
bool onClickDistribution = false;

String boost_str = "off";
uint8_t idx_boost = 0;
bool onClickBoost = false;

bool onClickSetpointUp = false;
bool onClickSetpointDown = false;

uint8_t curStatSmoke = 0;
uint8_t lastStatSmoke = 0;

uint8_t curStatState = 0;
uint8_t lastStatState = 0;

uint8_t curThermStat = 0;
uint8_t lastThermStat = 0;

void key_setpointup_cb(epdgui_args_vector_t &args)
{
    onClickSetpointUp = true;
    log_d("on click set point up");
    *((int*)(args[0])) = 1;
}

void key_setpointdown_cb(epdgui_args_vector_t &args)
{
    onClickSetpointDown = true;
    log_d("on click set point down");
    *((int*)(args[0])) = 1;
}

void key_blower_cb(epdgui_args_vector_t &args)
{
    idx_blower++;
    idx_blower%=4;
    espNowDataSent.blowerSpeedRmt = idx_blower;

    SetBlowerSpeed(idx_blower);

    switch(idx_blower)
    {
        case 0: 
            blower_str ="off";

            break;
        case 1:
            blower_str = "min";
            break;
        case 2:
            blower_str = "max";
            break;
        case 3:
            blower_str = "auto";
            break;
        default:
            blower_str ="off";
    }
    onClickBlower = true;
    log_d("on click blower");
    *((int*)(args[0])) = 1;
}

void key_distribution_cb(epdgui_args_vector_t &args)
{
    idx_distribution++;
    idx_distribution%=4;
    espNowDataSent.distribSpeedRmt = idx_distribution;
    SetDistributionSpeed(idx_distribution);

    switch(idx_distribution)
    {
        case 0: 
            distribution_str ="off";
            break;
        case 1:
            distribution_str = "min";
            break;
        case 2:
            distribution_str = "max";
            break;
        case 3:
            distribution_str = "auto";
            break;
        default:
            distribution_str ="off";
    }
    onClickDistribution = true;
    log_d("on click distribution");
    *((int*)(args[0])) = 1;
}

void key_boost_cb(epdgui_args_vector_t &args)
{

    idx_boost++;
    idx_boost%=2;
    espNowDataSent.boostStatRmt = idx_boost;
    SetBoostStat(idx_boost);
    
    switch(idx_boost)
    {
        case 0: 
            boost_str ="off";
            break;
        case 1:
            boost_str = "on";
            break;
        default:
            boost_str ="off";
    }
    onClickBoost = true;
    log_d("on click boost");
    *((int*)(args[0])) = 1;
}

void sw_high_cb(epdgui_args_vector_t &args) {
    SetSwitchState(SWITCH_HIGH);
    espNowDataSent.tStatRmt = true;
    SetThermostatValue(1);
    log_d("Select_Switch: High");
}

void sw_low_cb(epdgui_args_vector_t &args) {
    SetSwitchState(SWITCH_LOW);
    espNowDataSent.tStatRmt = false;
    SetThermostatValue(0);
    log_d("Select_Switch: low");
}

void Frame_Home::update_temp_room(float tmp)
{
    _room_area->fillCanvas(0);
    _room_area->setFreeFont(CR24);
    _room_area->setTextColor(15);
    _room_area->setTextSize(1);
    memset(buf_roomtemp, '\0', sizeof(buf_roomtemp));
    if (GetDegreesUnit() == DEGREES_CELSIUS) {
        sprintf(buf_roomtemp, "%.1f", tmp);
        _room_area->drawString(String(buf_roomtemp), 0, 0);
        _room_area->setTextSize(1);
        _room_area->setFreeFont(CB9);
        _room_area->drawString("o", 90, 0);
        _room_area->setTextSize(1);
        _room_area->setFreeFont(CB9);
        _room_area->drawString("C", 102, 5);
    }
    else{
        sprintf(buf_roomtemp, "%.1f", ((tmp) * 1.8) + 32);
        _room_area->drawString(String(buf_roomtemp), 0, 0);
        _room_area->setTextSize(1);
        _room_area->setFreeFont(CB9);
        _room_area->drawString("o", 90, 0);
        _room_area->setTextSize(1);
        _room_area->setFreeFont(CB9);
        _room_area->drawString("F", 102, 5);
    }
    _room_area->pushCanvas(230, 80, UPDATE_MODE_DU4);
    _time_update_room = millis();
}


void Frame_Home::update_setpoint(char arr_setpoint[], uint16_t val_setpoint)
{
    _setpoint_area->fillCanvas(0);
    _setpoint_area->setFreeFont(CB24);
    _setpoint_area->setTextColor(15);
    _setpoint_area->setTextSize(1);
    if (GetDegreesUnit() == DEGREES_CELSIUS) {
        sprintf(arr_setpoint, "%.1f", (float)(val_setpoint/10.0));
        _setpoint_area->drawString(String(arr_setpoint), 0, 0);

        _setpoint_area->setTextSize(1);
        _setpoint_area->setFreeFont(CB9);
        _setpoint_area->drawString("o", 125-35, 0);
        _setpoint_area->setTextSize(1);
        _setpoint_area->setFreeFont(CB9);
        _setpoint_area->drawString("C", 137-35, 5);
    }
    else{
        sprintf(arr_setpoint, "%.1f", ((float)(val_setpoint/10.0) * 1.8) + 32);
        _setpoint_area->drawString(String(arr_setpoint), 0, 0);

        _setpoint_area->setTextSize(1);
        _setpoint_area->setFreeFont(CB9);
        _setpoint_area->drawString("o", 125-35, 0);
        _setpoint_area->setTextSize(1);
        _setpoint_area->setFreeFont(CB9);
        _setpoint_area->drawString("F", 137-35, 5);
    }

    _setpoint_area->pushCanvas(230, 162, UPDATE_MODE_DU4);
    SetSetPoint(val_setpoint);
}


Frame_Home::Frame_Home(void) : Frame_Base(true) {
    _frame_name = "Frame_Home";
    //_frame_id   = 1;

#if 1 
    if(GetThermostatMode() == AUTOMATIC)
    {
        _key[KKeySetPointUp] = new EPDGUI_Button("sp up", 420, 33 + 36, 64, 64);
        _key[KKeySetPointUp]->CanvasNormal()->pushImage(0, 0, 64, 64, ImageResource_home_icon_up_64x64);
        *(_key[KKeySetPointUp]->CanvasPressed()) = *(_key[KKeySetPointUp]->CanvasNormal());
        _key[KKeySetPointUp]->CanvasPressed()->ReverseColor();
        _key[KKeySetPointUp]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void *)(&_is_run));
        _key[KKeySetPointUp]->Bind(EPDGUI_Button::EVENT_RELEASED, key_setpointup_cb);


        _key[KKeySetPointDown] = new EPDGUI_Button("sp down", 420, 33 + 5 + 64 + (64/4) + 36, 64, 64);
        _key[KKeySetPointDown]->CanvasNormal()->pushImage(0, 0, 64, 64, ImageResource_home_icon_down_64x64);
        *(_key[KKeySetPointDown]->CanvasPressed()) = *(_key[KKeySetPointDown]->CanvasNormal());
        _key[KKeySetPointDown]->CanvasPressed()->ReverseColor();
        _key[KKeySetPointDown]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void *)(&_is_run));
        _key[KKeySetPointDown]->Bind(EPDGUI_Button::EVENT_RELEASED, key_setpointdown_cb);
     }
#endif 

    _key[KKeyBlower] = new EPDGUI_Button("blower", 230, 536 - (72/2) + 10 + 32, 72, 72);
    _key[KKeyBlower]->CanvasNormal()->pushImage(0, 0, 72, 72, ImageResource_home_icon_blower_72x72);
    *(_key[KKeyBlower]->CanvasPressed()) = *(_key[KKeyBlower]->CanvasNormal());
    _key[KKeyBlower]->CanvasPressed()->ReverseColor();
    _key[KKeyBlower]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void *)(&_is_run));
    _key[KKeyBlower]->Bind(EPDGUI_Button::EVENT_RELEASED, key_blower_cb);

    _key[KKeyDistribution] = new EPDGUI_Button("distrib", 230, 686 - (72/2) + 10 + 32, 72, 72);
    _key[KKeyDistribution]->CanvasNormal()->pushImage(0, 0, 72, 72, ImageResource_home_icon_distribution_72x72);
    *(_key[KKeyDistribution]->CanvasPressed()) = *(_key[KKeyDistribution]->CanvasNormal());
    _key[KKeyDistribution]->CanvasPressed()->ReverseColor();
    _key[KKeyDistribution]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void *)(&_is_run));
    _key[KKeyDistribution]->Bind(EPDGUI_Button::EVENT_RELEASED, key_distribution_cb);

    _key[KKeyBoost] = new EPDGUI_Button("boost", 230, 836 - (72/2) + 10 + 32, 72, 72);
    _key[KKeyBoost]->CanvasNormal()->pushImage(0, 0, 72, 72, ImageResource_home_icon_boost_72x72);
    *(_key[KKeyBoost]->CanvasPressed()) = *(_key[KKeyBoost]->CanvasNormal());
    _key[KKeyBoost]->CanvasPressed()->ReverseColor();
    _key[KKeyBoost]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void *)(&_is_run));
    _key[KKeyBoost]->Bind(EPDGUI_Button::EVENT_RELEASED, key_boost_cb);


    _state_img = new M5EPD_Canvas(&M5.EPD);
    _state_img->createCanvas(64, 64);
    _state_img->pushImage(0, 0, 64, 64, ImageResource_home_icon_reload_ignition_64x64);

    _smoke_img = new M5EPD_Canvas(&M5.EPD);
    _smoke_img->createCanvas(64, 64);
    _smoke_img->pushImage(0, 0, 64, 64, ImageResource_home_icon_signal_lll_64x64);

    M5.SHT30.UpdateData();
    tmp_room = M5.SHT30.GetTemperature();
    tmp_room-=TEMP_ADJ;
    

    _frame_home = new M5EPD_Canvas(&M5.EPD);
    _room_area = new M5EPD_Canvas(&M5.EPD);
    _status_blower_area = new M5EPD_Canvas(&M5.EPD);
    _status_distrib_area = new M5EPD_Canvas(&M5.EPD);
    _status_boost_area = new M5EPD_Canvas(&M5.EPD);
    _setpoint_area = new M5EPD_Canvas(&M5.EPD);

    _room_area->createCanvas(130, 40 + 20);
    _room_area->fillCanvas(0);
    _room_area->setFreeFont(CR24);
    _room_area->setTextColor(15);
    _room_area->setTextSize(1);
    memset(buf_roomtemp, '\0', sizeof(buf_roomtemp));

#if 1 
    if (GetDegreesUnit() == DEGREES_CELSIUS) {
        sprintf(buf_roomtemp, "%.1f", tmp_room);
        _room_area->drawString(String(buf_roomtemp), 0, 0);
        _room_area->setTextSize(1);
        _room_area->setFreeFont(CB9);
        _room_area->drawString("o", 90, 0);
        _room_area->setTextSize(1);
        _room_area->setFreeFont(CB9);
        _room_area->drawString("C", 102, 5);
    }
    else
    {
        sprintf(buf_roomtemp, "%.1f", ((tmp_room) * 1.8) + 32);
        _room_area->drawString(String(buf_roomtemp), 0, 0);
        _room_area->setTextSize(1);
        _room_area->setFreeFont(CB9);
        _room_area->drawString("o", 90, 0);
        _room_area->setTextSize(1);
        _room_area->setFreeFont(CB9);
        _room_area->drawString("F", 102, 5);
    }
#endif

    _setpoint_area->createCanvas(85+95, 30 + 10);
    _setpoint_area->fillCanvas(0);
    _setpoint_area->setFreeFont(CB24);
    _setpoint_area->setTextColor(15);
    _setpoint_area->setTextSize(1);
    memset(buf_setpoint, '\0', sizeof(buf_setpoint));
    
    if (GetDegreesUnit() == DEGREES_CELSIUS) {
        sprintf(buf_setpoint, "%.1f",(float)(GetSetpoint()/10.0));
        _setpoint_area->drawString(String(buf_setpoint), 0, 0);
        _setpoint_area->setTextSize(1);
        _setpoint_area->setFreeFont(CB9);
        _setpoint_area->drawString("o", 125-35, 0);
        _setpoint_area->setTextSize(1);
        _setpoint_area->setFreeFont(CB9);
        _setpoint_area->drawString("C", 137-35, 5);
    }
    else
    {
        sprintf(buf_setpoint, "%.1f",((float)(GetSetpoint()/10.0)  * 1.8) + 32);
        _setpoint_area->drawString(String(buf_setpoint), 0, 0);
        _setpoint_area->setTextSize(1);
        _setpoint_area->setFreeFont(CB9);
        _setpoint_area->drawString("o", 125-35, 0);
        _setpoint_area->setTextSize(1);
        _setpoint_area->setFreeFont(CB9);
        _setpoint_area->drawString("F", 137-35, 5);
    }

    _status_blower_area->createCanvas(100, 30 + 10);
    _status_blower_area->fillCanvas(0);
    _status_blower_area->setFreeFont(CR24);
    _status_blower_area->setTextColor(15);
    _status_blower_area->setTextSize(1);
    _status_blower_area->drawString(blower_str, 0, 0);

    _status_distrib_area->createCanvas(100, 30 + 10);
    _status_distrib_area->fillCanvas(0);
    _status_distrib_area->setFreeFont(CR24);
    _status_distrib_area->setTextColor(15);
    _status_distrib_area->setTextSize(1);
    _status_distrib_area->drawString(distribution_str, 0, 0);

    _status_boost_area->createCanvas(100, 30 + 10);
    _status_boost_area->fillCanvas(0);
    _status_boost_area->setFreeFont(CR24);
    _status_boost_area->setTextColor(15);
    _status_boost_area->setTextSize(1);
    _status_boost_area->drawString(boost_str, 0, 0);

    _frame_home->createCanvas(540, 960-64);
    _frame_home->fillCanvas(0);
    _frame_home->setFreeFont(CR24);
    _frame_home->setTextColor(15);
    
    _frame_home->setTextSize(1);
    _frame_home->drawString("Room", 10, 16);

 #if 1   
    if(GetThermostatMode() ==AUTOMATIC)
    {
        _frame_home->setFreeFont(CB24);
        _frame_home->setTextSize(1);
        _frame_home->drawString("Set point", 10, 100);
        
    }
    else
    {
        _sw_high = new EPDGUI_Switch(2, 48 - 38, 140, 198, 61);
        _sw_low = new EPDGUI_Switch(2, 48 + 198 + 48 - 38, 140, 198, 61);

        _sw_high->SetLabel(0, "High");
        _sw_high->SetLabel(1, "High");
        _sw_high->Canvas(1)->ReverseColor();
        _sw_high->Bind(1, &sw_high_cb);

        _sw_low->SetLabel(0, "Low");
        _sw_low->SetLabel(1, "Low");
        _sw_low->Canvas(1)->ReverseColor();
        _sw_low->Bind(1, &sw_low_cb);

        _sw_mutex_group = new EPDGUI_MutexSwitch();
        _sw_mutex_group->Add(_sw_high);
        _sw_mutex_group->Add(_sw_low);

        if (GetSwitchState() == SWITCH_HIGH) {
            _sw_high->setState(1);
        }
        else
        {
            _sw_low->setState(1);
        }
    }
#endif
    
    _frame_home->drawFastHLine(10, 158, 520, 15);
    _frame_home->setFreeFont(CR24);
    _frame_home->drawString("State:", 10, 185);
    _frame_home->drawString("Smoke:", 10, 300);
    _frame_home->drawFastHLine(10, 360, 520, 15);

    _frame_home->drawString("Action", 200, 382);
    _frame_home->drawFastHLine(200, 432, 125, 15);

    _frame_home->drawString("Blower:", 10, 500);
    _frame_home->drawString("Distrib:", 10, 650);
    _frame_home->drawString("Boost:", 10, 800);

    _time             = 0;
    _next_update_time = 0;

 #if 1  
    if (GetLanguage() == LANGUAGE_FR) {
        exitbtn("Menu");
        _canvas_title->drawString("Piece", 270, 34-10);
    }
    else {
        exitbtn("Menu");
        _canvas_title->drawString("Home", 270, 34-10);
    }

    _key_exit->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void *)(&_is_run));
    _key_exit->Bind(EPDGUI_Button::EVENT_RELEASED, &Frame_Base::exit_cb);
 #endif   

}

Frame_Home::~Frame_Home(void) {

    delete _key[KKeySetPointUp];
    delete _key[KKeySetPointDown];
    delete _key[KKeyBlower];
    delete _key[KKeyDistribution];
    delete _key[KKeyBoost];
    delete _status_blower_area;
    delete _status_distrib_area;
    delete _status_boost_area;
    delete _frame_home;
    delete _room_area;
    delete _setpoint_area;
    delete _state_img;
    delete _smoke_img;
    delete _sw_high;
    delete _sw_low;
    delete _sw_mutex_group;
}

int Frame_Home::init(epdgui_args_vector_t &args) {

    _is_run = 1;
    M5.EPD.Clear();
    _shutdown_prompt_is_shown = false;

    idx_blower = GetBlowerSpeed();
    espNowDataSent.blowerSpeedRmt = idx_blower;
    switch(idx_blower)
    {
        case 0: 
            blower_str ="off";

            break;
        case 1:
            blower_str = "min";
            break;
        case 2:
            blower_str = "max";
            break;
        case 3:
            blower_str = "auto";
            break;
        default:
            blower_str ="off";
    }
    _status_blower_area->fillCanvas(0);
    _status_blower_area->drawString(blower_str, 0, 0);

    idx_distribution = GetDistributionSpeed();
    espNowDataSent.distribSpeedRmt = idx_distribution;

    switch(idx_distribution)
    {
        case 0: 
            distribution_str ="off";

            break;
        case 1:
            distribution_str = "min";
            break;
        case 2:
            distribution_str = "max";
            break;
        case 3:
            distribution_str = "auto";
            break;
        default:
            distribution_str ="off";
    }
    _status_distrib_area->fillCanvas(0);
    _status_distrib_area->drawString(distribution_str, 0, 0);


    idx_boost = GetBoostStat();
    espNowDataSent.boostStatRmt = idx_boost;
    switch(idx_boost)
    {
        case 0: 
            boost_str ="off";
            break;
        case 1:
            boost_str = "on";
            break;
        default:
            boost_str ="off";
    }
    _status_boost_area->fillCanvas(0);
    _status_boost_area->drawString(boost_str, 0, 0);

    if(GetThermostatMode() == MANUAL)
    {
        if (GetSwitchState() == SWITCH_HIGH) {
            _sw_high->setState(1);
            espNowDataSent.tStatRmt = true;
        }
        else
        {
            _sw_low->setState(1);
            espNowDataSent.tStatRmt = false;
        }
    }

    _time             = 0;
    _next_update_time = 0;
    _time_update_room = 0;
    _time_update_esp_now = 0;
    _setpoint_value = GetSetpoint();

    if(GetThermostatMode() == AUTOMATIC)
    {
        EPDGUI_AddObject(_key[KKeySetPointUp]);
        EPDGUI_AddObject(_key[KKeySetPointDown]);
    }
    else
    {
        EPDGUI_AddObject(_sw_mutex_group);
    }
    
    EPDGUI_AddObject(_key[KKeyBlower]);
    EPDGUI_AddObject(_key[KKeyDistribution]);
    EPDGUI_AddObject(_key[KKeyBoost]); 
    EPDGUI_AddObject(_key_exit);
    
    _frame_home->pushCanvas(0, 64, UPDATE_MODE_NONE);
    _room_area->pushCanvas(230, 80, UPDATE_MODE_NONE);
    if(GetThermostatMode() == AUTOMATIC)
    {
        _setpoint_area->pushCanvas(230, 162, UPDATE_MODE_NONE);
    }
    _status_blower_area->pushCanvas(400, 564, UPDATE_MODE_NONE);
    _status_distrib_area->pushCanvas(400, 714, UPDATE_MODE_NONE);
    _status_boost_area->pushCanvas(400, 864, UPDATE_MODE_NONE);
    _state_img->pushCanvas(230, 238, UPDATE_MODE_NONE);
    _smoke_img->pushCanvas(230, 338, UPDATE_MODE_NONE);
    _canvas_title->pushCanvas(0, 8, UPDATE_MODE_NONE);

    
    

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
    
    StatusBar(UPDATE_MODE_NONE, GetVoltageBattery());

    log_d("Init Frame_Home");
    return 3;
}

int Frame_Home::run() {
    Frame_Base::run();

    if ((millis() - _time_update_room) > (TIME_UPDATE_ROOM_MS/6)) {
        M5.SHT30.UpdateData();
        tmp_room = M5.SHT30.GetTemperature();
        tmp_room-=TEMP_ADJ;
        update_temp_room(tmp_room);
    }
    
    if(onClickBlower == true)
    {
        _status_blower_area->fillCanvas(0);
        _status_blower_area->setFreeFont(CR24);
        _status_blower_area->setTextColor(15);
        _status_blower_area->setTextSize(1);
        _status_blower_area->drawString(blower_str, 0, 0);
        _status_blower_area->pushCanvas(400, 564, UPDATE_MODE_DU4);
        onClickBlower = false;
    }
    else if(onClickDistribution == true)
    {
        _status_distrib_area->fillCanvas(0);
        _status_distrib_area->setFreeFont(CR24);
        _status_distrib_area->setTextColor(15);
        _status_distrib_area->setTextSize(1);
        _status_distrib_area->drawString(distribution_str, 0, 0);
        _status_distrib_area->pushCanvas(400, 714, UPDATE_MODE_DU4);
        onClickDistribution = false;
    }
    else if(onClickBoost == true)
    {
        _status_boost_area->fillCanvas(0);
        _status_boost_area->setFreeFont(CR24);
        _status_boost_area->setTextColor(15);
        _status_boost_area->setTextSize(1);
        _status_boost_area->drawString(boost_str, 0, 0);
        _status_boost_area->pushCanvas(400, 864, UPDATE_MODE_DU4);
        onClickBoost = false;
    }
    else if((onClickSetpointUp == true) && (GetThermostatMode() == AUTOMATIC))
    {
        _setpoint_value+=5;
        memset(buf_setpoint, '\0', sizeof(buf_setpoint));
        update_setpoint(buf_setpoint, _setpoint_value);
        onClickSetpointUp = false;
    }
    else if((onClickSetpointDown == true) && (GetThermostatMode() == AUTOMATIC))
    {
        _setpoint_value-=5;
        memset(buf_setpoint, '\0', sizeof(buf_setpoint));
        update_setpoint(buf_setpoint, _setpoint_value);
        onClickSetpointDown = false;
    }

    if(GetThermostatMode() == AUTOMATIC)
    {
        if(_setpoint_value > ((uint16_t)((tmp_room) * 10)))
        { 
            espNowDataSent.tStatRmt = true;
            curThermStat = 1;
        }
        else
        {
            espNowDataSent.tStatRmt = false;
            curThermStat = 2;
        }

        if(lastThermStat != curThermStat)
        {
            if(curThermStat == 1)
            {
                SetThermostatValue(1);
            }
            else if(curThermStat == 2)
            {
                SetThermostatValue(0);
            }
        }
        lastThermStat = curThermStat;
    }

    if(dataDebug.PartCurr)
    {
        if(((float)(dataDebug.PartCH0ON * 1.0) / dataDebug.PartCurr) < 100)
        {
            curStatSmoke = 1;
        }
        else if((((float)(dataDebug.PartCH0ON * 1.0) / dataDebug.PartCurr) >= 100)  && (((float)(dataDebug.PartCH0ON * 1.0) / dataDebug.PartCurr) < 250))
        {
            curStatSmoke = 2;
        }
        else if(((float)(dataDebug.PartCH0ON * 1.0) / dataDebug.PartCurr) >= 250)
        {
            curStatSmoke = 3;
        }

        if(lastStatSmoke != curStatSmoke)
        {
            if(curStatSmoke == 1)
            {
                _smoke_img->fillCanvas(0);
                _smoke_img->pushImage(0, 0, 64, 64, ImageResource_home_icon_signal_hll_64x64);
                _smoke_img->pushCanvas(230, 338, UPDATE_MODE_GL16);
            }
            else if(curStatSmoke == 2)
            {
                _smoke_img->fillCanvas(0);
                _smoke_img->pushImage(0, 0, 64, 64, ImageResource_home_icon_signal_hhl_64x64);
                _smoke_img->pushCanvas(230, 338, UPDATE_MODE_GL16);
            }
            else if(curStatSmoke == 3)
            {
                _smoke_img->fillCanvas(0);
                _smoke_img->pushImage(0, 0, 64, 64, ImageResource_home_icon_signal_hhh_64x64);
                _smoke_img->pushCanvas(230, 338, UPDATE_MODE_GL16);
            }
        }
        lastStatSmoke = curStatSmoke;
    }

    if((dataDebug.Tbaffle >= 10)  && (dataDebug.Tbaffle < 500))
    {
        curStatState = 1;
    }
    else if((dataDebug.Tbaffle >= 500) && (dataDebug.Tbaffle < 650))
    {
        curStatState = 2;
    }
    else if(dataDebug.Tbaffle > 650)
    {
        curStatState = 3;
    }
    
    if(lastStatState != curStatState)
    {
        if(curStatState == 1)
        {
            _state_img->fillCanvas(0);
            _state_img->pushImage(0, 0, 64, 64, ImageResource_home_icon_coal_64x64);
            _state_img->pushCanvas(230, 238, UPDATE_MODE_GL16);
        }
        else if(curStatState == 2)
        {
            _state_img->fillCanvas(0);
            _state_img->pushImage(0, 0, 64, 64, ImageResource_home_icon_flame_low_64x64);
            _state_img->pushCanvas(230, 238, UPDATE_MODE_GL16);
        }
        else if(curStatState == 3)
        {
            _state_img->fillCanvas(0);
            _state_img->pushImage(0, 0, 64, 64, ImageResource_home_icon_flame_high_64x64);
            _state_img->pushCanvas(230, 238, UPDATE_MODE_GL16);
        }
    }
    lastStatState = curStatState;

    if(!strcmp(dataDebug.State, "BOOST"))
    {
        idx_boost = 0;
        boost_str ="off";
        if(espNowDataSent.boostStatRmt != idx_boost)
        {
            SetBoostStat(idx_boost);
            espNowDataSent.boostStatRmt = idx_boost;
        }
    }

    

#if 1 
    if ((millis() - _time_update_esp_now) > (100U)) {
        espNowDataSent.tStatRmt = GetThermostatValue();
        espNowDataSent.blowerSpeedRmt = GetBlowerSpeed();
        espNowDataSent.distribSpeedRmt = GetDistributionSpeed();
        espNowDataSent.boostStatRmt = GetBoostStat();

        log_d("home.tStatRmt: %d",espNowDataSent.tStatRmt);
        log_d("home.blowerSpeedRmt: %d",espNowDataSent.blowerSpeedRmt);
        log_d("home.distribSpeedRmt: %d",espNowDataSent.distribSpeedRmt);
        log_d("home.boostStatRmt: %d",espNowDataSent.boostStatRmt);
        sendDataToEspNow(espNowDataSent);
        _time_update_esp_now = millis();
    }
#endif

    return 1;
}