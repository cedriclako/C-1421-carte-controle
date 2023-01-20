#ifndef _SLEEPDATA_H_
#define _SLEEPDATA_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SLEEPDATA_RECORD_MAGICBYTE 0xAA
#define SLEEPDATA_RECORD_SIZE (16)

#define SLEEPDATA_RECORDS_COUNT (4096/SLEEPDATA_RECORD_SIZE)

typedef union
{
    struct
    {
        uint8_t u8MagicByte;
        uint8_t u8LastChannel;
    } sData;
    uint8_t u8Datas[SLEEPDATA_RECORD_SIZE];
} SLEEPDATA_URecord;

void SLEEPDATA_Init();

bool SLEEPDATA_ReadLastRecord(SLEEPDATA_URecord* pULastRecord);

bool SLEEPDATA_WriteRecord(SLEEPDATA_URecord* pULastRecord);

#ifdef __cplusplus
}
#endif

#endif