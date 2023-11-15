/*
 * CRC32Calculator.c
 *
 *  Created on: Nov 10, 2023
 *      Author: mcarrier
 */
#ifndef _CRC32_H_
#define _CRC32_H_

#include <stdint.h>

// void CRC32_AccumulateReset();
// uint32_t CRC32_GetAccumulateResult();
// uint32_t CRC32_Accumulate(uint8_t u8Datas[], uint32_t u32StartOffset, uint32_t u32length);
uint32_t CRC32_CalculateArray(uint8_t u8Datas[], uint32_t u32StartOffset, uint32_t u32length);

#endif
