/*
 * ParameterFile.c
 *
 *  Created on: 13 déc. 2022
 *      Author: mcarrier
 */
#include <string.h>
#include "ParameterFileLib.h"
#include "ParamFile.h"

static const PFL_SParameterItem* GetParameterEntryByKey(const PFL_SHandle* pHandle, const char* szKey);
static PFL_ESETRET ValidateValueInt32(const PFL_SHandle* pHandle, const PFL_SParameterItem* pParameterFile, int32_t s32Value);

void PFL_Init(PFL_SHandle* pHandle, const PFL_SParameterItem* pParameterEntries, uint32_t u32ParameterEntryCount)
{
	pHandle->pParameterEntries = pParameterEntries;
	pHandle->u32ParameterEntryCount = u32ParameterEntryCount;
}

int32_t PFL_GetValueInt32(const PFL_SHandle* pHandle, const char* szName)
{
	const PFL_SParameterItem* pEnt = GetParameterEntryByKey(pHandle, szName);
   // int32_t s32 = 0;

    // TODO: Code to retrieve value ...
	// ValidateValueInt32

    return pEnt->uType.sInt32.s32Default;
}

PFL_ESETRET PFL_SetValueInt32(const PFL_SHandle* pHandle, const char* szName, int32_t s32NewValue)
{
	return PFL_ESETRET_OK;
}

static PFL_ESETRET ValidateValueInt32(const PFL_SHandle* pHandle, const PFL_SParameterItem* pParameterFile, int32_t s32Value)
{
    assert(pParameterFile != NULL && pParameterFile->eType == PFL_TYPE_Int32);
	if (pParameterFile->uType.sInt32.s32Min < s32Value || pParameterFile->uType.sInt32.s32Max > s32Value)
		return PFL_ESETRET_InvalidRange;
	return PFL_ESETRET_OK;
}

static const PFL_SParameterItem* GetParameterEntryByKey(const PFL_SHandle* pHandle, const char* szKey)
{
    for(int i = 0; i < pHandle->u32ParameterEntryCount; i++)
    {
    	const PFL_SParameterItem* pParamItem = &pHandle->pParameterEntries[i];
        if (strcmp(pParamItem->szKey, szKey) == 0)
        {
            return pParamItem;
        }
    }
    return NULL;
}
