#ifndef _ESCREEH_H_
#define _ESCREEH_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    ESCREEN_Invalid = -1,  // Default value, nothing loaded yet.

    ESCREEN_HomeUsermode = 0,

    ESCREEN_UIPoweredOffView,
    ESCREEN_PoweringOn,

    ESCREEN_Settings,

    ESCREEN_Count
} ESCREEN;

#ifdef __cplusplus
}
#endif

#endif