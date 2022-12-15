#ifndef _EVENTSOURCE_H_
#define _EVENTSOURCE_H_

#include "esp_event.h"
#include "esp_timer.h"

ESP_EVENT_DECLARE_BASE(MAINAPP_EVENT);         // declaration of the task events family

enum {
    REQUESTCONFIGRELOAD_EVENT,
};

#endif