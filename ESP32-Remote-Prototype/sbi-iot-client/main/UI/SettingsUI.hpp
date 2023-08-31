#ifndef _SETTINGSUI_H_
#define _SETTINGSUI_H_

#include "CommonUI.hpp"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    COMMONUI_SButton sBtClose;    
} SETTINGSUI_SHandle;

extern const COMMONUI_SConfig SETTINGSUI_g_sConfig;

#ifdef __cplusplus
}
#endif

#endif