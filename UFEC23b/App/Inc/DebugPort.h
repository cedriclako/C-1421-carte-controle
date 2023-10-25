#ifndef DEBUG_PORT_H
#define DEBUG_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

extern int _write(int file, char *ptr, int len);

void LOG(const char* tag, const char *format, ...);

#define LOG(_moduleTag, _format, ...) LOG(_moduleTag, _format,  ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif
