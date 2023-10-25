/*
 * FlashMap.h
 *
 *  Created on: Sep 26, 2023
 *      Author: mcarrier
 */

#ifndef INC_FLASHMAP_H_
#define INC_FLASHMAP_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "stm32f1xx_hal.h"

typedef enum
{
	FMAP_EPARTITION_AppBin = 0,
	FMAP_EPARTITION_Parameters,

	FMAP_EPARTITION_Count
} FMAP_EPARTITION;

typedef struct
{
	int32_t s32SectorStart;
	int32_t s32SectorCount;
} FMAP_SPart;

#define FMAP_INTERNALFLASH_SIZE (FLASH_BANK1_END - FLASH_BASE + 1)

#define FMAP_PARAMETER_SECTOR_COUNT (2)
#define FMAP_PARAMETER_SECTOR_LEN (FMAP_PARAMETER_SECTOR_COUNT * FLASH_PAGE_SIZE)

const FMAP_SPart* FMAP_GetPartition(FMAP_EPARTITION ePartition);

bool FMAP_ErasePartition(FMAP_EPARTITION ePartition);

bool FMAP_WriteAtPartition(FMAP_EPARTITION ePartition, uint32_t u32RelativeAddr, const uint8_t u8SrcData[], uint32_t u32SrcSize);

const uint8_t* FMAP_GetMemoryAddr(FMAP_EPARTITION ePartition);

#endif /* INC_FLASHMAP_H_ */
