#include "FlashMap.h"
#include <assert.h>

static const FLASHMAP_SPartition FLASHMAP_g_sPartitions[] =
{
	[FLASHMAP_EPARTITION_App] 			= { .u32PageStart = 0,   .u32PageCount = 124 },
	[FLASHMAP_EPARTITION_Parameter] 	= { .u32PageStart = 124, .u32PageCount = 3 },
	[FLASHMAP_EPARTITION_BootInfo] 		= { .u32PageStart = 127, .u32PageCount = 1 }
};

static_assert( (sizeof(FLASHMAP_g_sPartitions)/sizeof(FLASHMAP_g_sPartitions[0])) == FLASHMAP_EPARTITION_Count, "Partition count");

const FLASHMAP_SPartition* GetPartition(FLASHMAP_EPARTITION ePartition)
{
	if (ePartition < 0 && ePartition > FLASHMAP_EPARTITION_Count)
		return NULL;
	return &FLASHMAP_g_sPartitions[ePartition];
}
