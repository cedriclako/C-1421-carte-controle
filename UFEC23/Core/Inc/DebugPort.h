#ifndef DEBUG_PORT_H
#define DEBUG_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

extern int _write(int file, char *ptr, int len);

#ifdef __cplusplus
}
#endif

#endif
