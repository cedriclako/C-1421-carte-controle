/*
 * MemBlock.h
 *
 *  Created on: 31 janv. 2023
 *      Author: mcarrier
 */

#ifndef SRC_MEMBLOCK_H_
#define SRC_MEMBLOCK_H_

#include <stdint.h>
#include <stddef.h>

typedef struct
{
	uint32_t u32SensorParticleRXCount;
} MEMBLOCK_SData;

void MEMBLOCK_Init();

extern MEMBLOCK_SData g_MEMBLOCK_sInstance;

#endif /* SRC_MEMBLOCK_H_ */
