#include "FlashMap.h"
#include <assert.h>

const FLASHMAP_SPartition FLASHMAP_g_sPartitions[] =
{
	[FLASHMAP_EPARTITION_App] 			= { .u32PageAddress = 0, .u32PageCount = 0 },
	[FLASHMAP_EPARTITION_Parameter] 	= { .u32PageAddress = 0, .u32PageCount = 0 }
};

static_assert( (sizeof(FLASHMAP_g_sPartitions)/sizeof(FLASHMAP_g_sPartitions[0])) == FLASHMAP_EPARTITION_Count, "Partition count");
