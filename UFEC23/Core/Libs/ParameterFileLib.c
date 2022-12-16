/*
 * ParameterFile.c
 *
 *  Created on: 13 déc. 2022
 *      Author: mcarrier
 */
#include <string.h>
#include <stdio.h>
#include "ParameterFileLib.h"
#include "cJSON/cJSON.h"

#define JSON_ENTRY_KEY_NAME "key"
#define JSON_ENTRY_VALUE_NAME "value"

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

int32_t PFL_ExportToJSON(const PFL_SHandle* pHandle, char* szBuffer, int32_t s32MaxLen)
{
	int n = 0;
	szBuffer[n] = '\0';
	const int c1 = snprintf(szBuffer+n, (size_t)(s32MaxLen - n - 1), "[\r\n");
	if (c1 <= 0)
		return -1;
	n += c1;

	for(int i = 0; i < pHandle->u32ParameterEntryCount; i++)
	{
		const PFL_SParameterItem* pEnt = &pHandle->pParameterEntries[i];

		// Generate without using external library is fast
		// I'm aware there are pitfail before string aren't escaped but since it's very specific usecase it's good enough.
		const int newEntryCnt = snprintf(szBuffer+n, (size_t)(s32MaxLen - n - 1),
			"{ \"key\":\"%s\",\"desc\":\"%s\",\"t\":%d,\"def\":%d,\"min\":%d,\"max\":%d }\r\n",
			/*0*/pEnt->szKey,
			/*1*/pEnt->szDesc,
			/*2*/(int)pEnt->eType,
			/*3*/(int)pEnt->uType.sInt32.s32Default,
			/*4*/(int)pEnt->uType.sInt32.s32Min,
			/*5*/(int)pEnt->uType.sInt32.s32Max);
		if (newEntryCnt <= 0)
			return -1;
		n += newEntryCnt;

		if (i + 1 < pHandle->u32ParameterEntryCount)
		{
			const int sepc = snprintf(szBuffer+n, (size_t)(s32MaxLen - n - 1), ",");
			if (sepc <= 0)
				return -1;
			n += sepc;
		}
	}

	const int c2 = snprintf(szBuffer+n, (size_t)(s32MaxLen - n - 1), "]");
	if (c2 <= 0)
		return -1;
	n += c2;

	return 0;
}

bool PFL_ImportFromJSON(const PFL_SHandle* pHandle, const char* szBuffer)
{
	bool bRet = false;
	cJSON* pRoot = cJSON_Parse(szBuffer);
	if (pRoot == NULL)
	{
		goto ERROR;
	}

	// Two pass process, one to validate and one to write
    for(int pass = 0; pass < 2; pass++)
    {
        const bool bIsDryRun = pass == 0;

        for(int i = 0; i < cJSON_GetArraySize(pRoot); i++)
        {
            cJSON* pEntryJSON = cJSON_GetArrayItem(pRoot, i);

            cJSON* pKeyJSON = cJSON_GetObjectItemCaseSensitive(pEntryJSON, JSON_ENTRY_KEY_NAME);
            if (pKeyJSON == NULL || !cJSON_IsString(pKeyJSON))
            {
                goto ERROR;
            }

            cJSON* pValueJSON = cJSON_GetObjectItemCaseSensitive(pEntryJSON, JSON_ENTRY_VALUE_NAME);
            if (pValueJSON == NULL)
            {
                // We just ignore changing the setting if the value property is not there.
                // it allows us to handle secret cases.
                continue;
            }

    		const char* szKey = pKeyJSON->valuestring;

    		const PFL_SParameterItem* pParamItem = GetParameterEntryByKey(pHandle, szKey);
    		// Just ignore if it doesn't exists
    		if (pParamItem == NULL)
    			continue;

    		// Int32
    		if (pParamItem->eType == PFL_TYPE_Int32)
    		{
                if (!cJSON_IsNumber(pValueJSON))
                {
                    // JSON value type is invalid, not a number
                    goto ERROR;
                }

                const int32_t s32NewValue = pValueJSON->valueint;

    			if (bIsDryRun)
    			{
    				if (ValidateValueInt32(pHandle,  pParamItem, s32NewValue) != PFL_ESETRET_OK)
    					goto ERROR;
    			}
    			else
    			{
    				// It has been validate on the first run so it should work here. If it doesn't it indicate a bug somewhere in the library.
    				if (PFL_SetValueInt32(pHandle, szKey, s32NewValue) != PFL_ESETRET_OK)
    					goto ERROR;
    			}
    		}
        }
    }

    bRet = true;
    goto END;
    ERROR:
    bRet = false;
    END:
    cJSON_free(pRoot);
    return bRet;
}
