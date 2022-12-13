/*
 * FlashMap.h
 *
 *  Created on: 13 déc. 2022
 *      Author: mcarrier
 */

#ifndef INC_FLASHMAP_H_
#define INC_FLASHMAP_H_

#include <stdint.h>
#include <stddef.h>

typedef enum
{
	FLASHMAP_EPARTITION_App,
	FLASHMAP_EPARTITION_Parameter,

	FLASHMAP_EPARTITION_Count
} FLASHMAP_EPARTITION;

typedef struct
{
	uint32_t u32PageAddress;
	uint32_t u32PageCount;
} FLASHMAP_SPartition;

extern const FLASHMAP_SPartition FLASHMAP_g_sPartitions[];

#endif
