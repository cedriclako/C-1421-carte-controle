#ifndef _ESCREEH_H_
#define _ESCREEH_H_

typedef enum
{
    ESCREEN_Invalid = -1,  // Default value, nothing loaded yet.

    ESCREEN_HomeReadOnly = 0,
    ESCREEN_HomeUsermode,
    ESCREEN_PoweringOn,

    ESCREEN_Settings,

    ESCREEN_Count
} ESCREEN;

#endif