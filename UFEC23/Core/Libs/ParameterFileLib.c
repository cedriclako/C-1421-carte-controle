/*
 * ParameterFile.c
 *
 *  Created on: 13 déc. 2022
 *      Author: mcarrier
 */
#include <string.h>
#include "ParameterFileLib.h"

static const PFL_SParameterItem* GetParameterEntryByKey(const PFL_SHandle* pHandle, const char* szKey);
static PFL_ESETRET ValidateValueInt32(const PFL_SHandle* pHandle, const PFL_SParameterItem* pParameterFile, int32_t s32Value);

void PFL_Init(PFL_SHandle* pHandle, const PFL_SParameterItem* pParameterEntries, uint32_t u32ParameterEntryCount, const PFL_SConfig* psConfig)
{
	pHandle->pParameterEntries = pParameterEntries;
	pHandle->u32ParameterEntryCount = u32ParameterEntryCount;

	pHandle->psConfig = psConfig;
}

void PFL_LoadAll(PFL_SHandle* pHandle)
{
	if (pHandle->psConfig->ptrLoadAll != NULL)
		pHandle->psConfig->ptrLoadAll(pHandle);

	// Verify variables and load default value if necessary
	for(int i = 0; i < pHandle->u32ParameterEntryCount; i++)
	{
		const PFL_SParameterItem* pEnt = &pHandle->pParameterEntries[i];

		if (pEnt->eType == PFL_TYPE_Int32)
		{
			int32_t* ps32Value = ((int32_t*)pEnt->vdVar);
			const PFL_ESETRET eSetRet = ValidateValueInt32(pHandle, pEnt, *ps32Value);
			if (eSetRet != PFL_ESETRET_OK)
			{
				*ps32Value = pEnt->uType.sInt32.s32Default;
			}
		}
	}
}

void PFL_CommitAll(PFL_SHandle* pHandle)
{
	if (pHandle->psConfig->ptrCommitAll != NULL)
		pHandle->psConfig->ptrCommitAll(pHandle);
}

PFL_ESETRET PFL_GetValueInt32(const PFL_SHandle* pHandle, const char* szName, int32_t* psOut32Value)
{
	const PFL_SParameterItem* pEnt = GetParameterEntryByKey(pHandle, szName);
	if (pEnt == NULL || pEnt->eType != PFL_TYPE_Int32 || pEnt->vdVar == NULL)
		return PFL_ESETRET_EntryNoFound;

	const int32_t* ps32Value = ((int32_t*)pEnt->vdVar);
	// We don't need to validate on read finally because the LoadAll function will put load or put default values
/*	const PFL_ESETRET eValidateRet = ValidateValueInt32(pHandle, pEnt, *ps32Value);
	if (eValidateRet != PFL_ESETRET_OK)
	{
		*psOut32Value = pEnt->uType.sInt32.s32Default;
		return eValidateRet;
	}*/
	*psOut32Value = *ps32Value;
	return PFL_ESETRET_OK;
}

PFL_ESETRET PFL_SetValueInt32(const PFL_SHandle* pHandle, const char* szName, int32_t s32NewValue)
{
	const PFL_SParameterItem* pEnt = GetParameterEntryByKey(pHandle, szName);
	if (pEnt == NULL || pEnt->eType != PFL_TYPE_Int32 || pEnt->vdVar == NULL)
		return PFL_ESETRET_EntryNoFound;

	int32_t* ps32Value = ((int32_t*)pEnt->vdVar);
	const PFL_ESETRET eValidateRet = ValidateValueInt32(pHandle, pEnt, *ps32Value);
	if (eValidateRet != PFL_ESETRET_OK)
		return eValidateRet;
	// We can record if it pass validation step
	*ps32Value = s32NewValue;
	return PFL_ESETRET_OK;
}

static PFL_ESETRET ValidateValueInt32(const PFL_SHandle* pHandle, const PFL_SParameterItem* pParameterFile, int32_t s32Value)
{
    assert(pParameterFile != NULL && pParameterFile->eType == PFL_TYPE_Int32);
	if (s32Value < pParameterFile->uType.sInt32.s32Min || s32Value > pParameterFile->uType.sInt32.s32Max)
		return PFL_ESETRET_InvalidRange;
	return PFL_ESETRET_OK;
}

static const PFL_SParameterItem* GetParameterEntryByKey(const PFL_SHandle* pHandle, const char* szKey)
{
    for(int i = 0; i < pHandle->u32ParameterEntryCount; i++)
    {
    	const PFL_SParameterItem* pParamItem = &pHandle->pParameterEntries[i];
        if (strcmp(pParamItem->szKey, szKey) == 0)
            return pParamItem;
    }
    return NULL;
}
