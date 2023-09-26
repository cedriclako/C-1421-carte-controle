/*
 * MemBlock.c
 *
 *  Created on: 31 janv. 2023
 *      Author: mcarrier
 */
#include "MemBlock.h"

MEMBLOCK_SData g_MEMBLOCK_sInstance;

void MEMBLOCK_Init()
{
	memset(&g_MEMBLOCK_sInstance, 0, sizeof(g_MEMBLOCK_sInstance));
}
