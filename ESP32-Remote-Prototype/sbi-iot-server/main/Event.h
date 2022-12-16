#ifndef _EVENTSOURCE_H_
#define _EVENTSOURCE_H_

#include "esp_event.h"
#include "esp_timer.h"

ESP_EVENT_DECLARE_BASE(MAINAPP_EVENT);         // declaration of the task events family

enum {
    REQUESTCONFIGRELOAD_EVENT,

    //UFEC23_CONNECTED_EVENT,
    // UFEC23_DISCONNECTED_EVENT
};

extern esp_event_loop_handle_t EVENT_g_LoopHandle;

#endif