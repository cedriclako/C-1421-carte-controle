/*
 * FlashMap.c
 *
 *  Created on: Sep 26, 2023
 *      Author: mcarrier
 */
#include <assert.h>
#include "FlashMap.h"

#define INTERNALFLASH_DOUBLEWORDLEN (4)

// Trailer, used to identify the app-bin size at runtime.
static const uint8_t m_u8Trailers[16] __attribute__((__section__(".TrailerMarker"))) = { 'S', 'B', 'I', 0xDF, 0x2F, 0x87, 0x68, 0x29, 0xe3, 0x43, 0x37, 'T', 'R', 'A', 'I', 'L' };

static const FMAP_SPart m_sPartitions[] =
{
	[FMAP_EPARTITION_AppBin] = 		{ .s32SectorStart = 0, 	 .s32SectorCount = 124 },
	[FMAP_EPARTITION_Parameters] = 	{ .s32SectorStart = 124, .s32SectorCount = FMAP_PARAMETER_SECTOR_COUNT },
	[FMAP_EPARTITION_Extras] 	= 	{ .s32SectorStart = 126, .s32SectorCount = FMAP_EXTRA_SECTOR_COUNT }
};

static_assert(FMAP_INTERNALFLASH_SIZE == 0x00040000, "These partitions are made for a 256 KB internal flash. Please adjust it if changed");

const FMAP_SPart* FMAP_GetPartition(FMAP_EPARTITION ePartition)
{
	if ((int)ePartition >= (int)FMAP_EPARTITION_Count)
		return NULL;
	return &m_sPartitions[(int)ePartition];
}

bool FMAP_ErasePartition(FMAP_EPARTITION ePartition)
{
	HAL_FLASH_Unlock();

	const FMAP_SPart* pPartition = FMAP_GetPartition(ePartition);
	if (pPartition == NULL)
		goto ERROR;

    /* Configure flash erase of sector */
    FLASH_EraseInitTypeDef EraseInitStruct;
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.PageAddress = FLASH_BASE + (pPartition->s32SectorStart * FLASH_PAGE_SIZE);
    EraseInitStruct.NbPages = pPartition->s32SectorCount;

    __disable_irq();
    uint32_t PageError = 0;
    if (HAL_OK != HAL_FLASHEx_Erase(&EraseInitStruct, &PageError))
    {
       __enable_irq();
       goto ERROR;
    }
    __enable_irq();
    HAL_FLASH_Lock();
    return true;
    ERROR:
    HAL_FLASH_Lock();
	return false;
}

const uint8_t* FMAP_GetMemoryAddr(FMAP_EPARTITION ePartition)
{
	const FMAP_SPart* pPartition = FMAP_GetPartition(ePartition);
	if (pPartition == NULL)
		return false;

	return (const uint8_t*)(FLASH_BASE + (pPartition->s32SectorStart * FLASH_PAGE_SIZE));
}

bool FMAP_WriteAtPartition(FMAP_EPARTITION ePartition, uint32_t u32RelativeAddr, const uint8_t u8SrcData[], uint32_t u32SrcSize)
{
	bool ret = false;

	const FMAP_SPart* pPartition = FMAP_GetPartition(ePartition);
	if (pPartition == NULL)
		return false;

	const uint32_t u32PartitionStartAddr = FLASH_BASE + (pPartition->s32SectorStart * FLASH_PAGE_SIZE) + u32RelativeAddr;

	assert((u32PartitionStartAddr % INTERNALFLASH_DOUBLEWORDLEN) == 0);
	assert((u32SrcSize % INTERNALFLASH_DOUBLEWORDLEN) == 0);

	//We have to unlock flash module to get control of registers
	HAL_FLASH_Unlock();

	for(uint32_t i = 0; i < u32SrcSize; i += INTERNALFLASH_DOUBLEWORDLEN)
	{
		// Flash address start at the constant
		const uint32_t u32AddressFlash = u32PartitionStartAddr + i;

		// Here we program the flash byte by byte
		const uint32_t data32 = (u8SrcData[i+3] << 24) | (u8SrcData[i+2] << 16) | (u8SrcData[i+1] << 8) | u8SrcData[i];

		const HAL_StatusTypeDef drvStatus = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, u32AddressFlash, data32 );
		if (HAL_OK != drvStatus)
			goto ERROR;
	}
	ret = true;
	goto END;
	ERROR:
	ret = false;
	END:
	HAL_FLASH_Lock();
	return ret;
}
