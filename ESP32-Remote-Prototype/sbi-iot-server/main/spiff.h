#ifndef _SPIFF_H_
#define _SPIFF_H_

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "fwconfig.h"

#define SPIFF_FILE_PATH_MAX (160)

void SPIFF_Init(void);

bool SPIFF_GetIsInitialized();

bool SPIFF_GetFileSystemInfo(size_t* pTotalBytes, size_t* pUsedBytes);

#endif