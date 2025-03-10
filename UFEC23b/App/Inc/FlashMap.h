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
#include <string.h>
#include "stm32f1xx_hal.h"
#include "BinaryMarker.h"

typedef enum
{
	FMAP_EPARTITION_AppBin = 0,
	FMAP_EPARTITION_Parameters,
	FMAP_EPARTITION_Extras,		/*! @brief Extra space for future use */
	
	FMAP_EPARTITION_Count
} FMAP_EPARTITION;

typedef struct
{
	int32_t s32SectorStart;
	int32_t s32SectorCount;
} FMAP_SPart;

typedef struct
{
  uint32_t u32Size;
  uint32_t u32CRC32;
} FMAP_SFileInfo;

#define FMAP_INTERNALFLASH_SIZE (FLASH_BANK1_END - FLASH_BASE + 1)

#define FMAP_PARAMETER_SECTOR_COUNT (2)
#define FMAP_EXTRA_SECTOR_COUNT (2)
#define FMAP_PARAMETER_SECTOR_LEN (FMAP_PARAMETER_SECTOR_COUNT * FLASH_PAGE_SIZE)

#define FMAP_SECTOR2BYTES(_sectorNumber) (_sectorNumber * FLASH_PAGE_SIZE)

void FMAP_Init();

const FMAP_SPart* FMAP_GetPartition(FMAP_EPARTITION ePartition);

bool FMAP_ErasePartition(FMAP_EPARTITION ePartition);

bool FMAP_WriteAtPartition(FMAP_EPARTITION ePartition, uint32_t u32RelativeAddr, const uint8_t u8SrcData[], uint32_t u32SrcSize);

const uint8_t* FMAP_GetMemoryAddr(FMAP_EPARTITION ePartition);

const FMAP_SFileInfo* FMAP_GetAppFileInfo();

#endif /* INC_FLASHMAP_H_ */
