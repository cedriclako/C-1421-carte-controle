#ifndef _SBIIOTBASEPROTOCOL_H_
#define _SBIIOTBASEPROTOCOL_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SBIIOTBASEPROTOCOL_MAGIC_CMD { (uint8_t)'S', (uint8_t)'B', (uint8_t)'I', (uint8_t)0x55 }
#define SBIIOTBASEPROTOCOL_MAGIC_CMD_LEN (4)

// ESP_NOW_MAX_DATA_LEN = 250, keep 10 bytes for maginc number and reserved
#define SBIIOTBASEPROTOCOL_MAXPAYLOADLEN (240)

#ifdef __cplusplus
}
#endif

#endif