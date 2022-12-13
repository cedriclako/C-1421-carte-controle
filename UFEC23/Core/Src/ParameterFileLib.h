/*
 * ParameterFile.h
 *
 *  Created on: 13 déc. 2022
 *      Author: mcarrier
 */

#ifndef INC_PARAMETERFILELIB_H_
#define INC_PARAMETERFILELIB_H_

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stddef.h>

typedef enum
{
	PFL_TYPE_Float,
	PFL_TYPE_Int32
} PFL_TYPE;

typedef struct
{
	const char* szKey;
	const char* szDesc;
	PFL_TYPE eType;
	const void* vdArgument;
	union
	{
		struct
		{
			float fDefault;
			float fMin;
			float fMax;
		} sFloat;
		struct
		{
			int32_t s32Default;
			int32_t s32Min;
			int32_t s32Max;
		} sInt32;
	} uType;
} PFL_SParameterItem;

typedef struct
{
	const PFL_SParameterItem* pParameterEntries;
	uint32_t u32ParameterEntryCount;
} PFL_SHandle;


typedef enum
{
    PFL_ESETRET_OK = 0,
    PFL_ESETRET_CannotSet = 1,
    PFL_ESETRET_InvalidRange = 2,
    PFL_ESETRET_ValidatorFailed = 3,
} PFL_ESETRET;


void PFL_Init(PFL_SHandle* pHandle, const PFL_SParameterItem* pParameterEntries, uint32_t u32ParameterEntryCount);

int32_t PFL_GetValueInt32(const PFL_SHandle* pHandle, const char* szName);

PFL_ESETRET PFL_SetValueInt32(const PFL_SHandle* pHandle, const char* szName, int32_t s32NewValue);

#endif /* INC_PARAMETERFILE_H_ */
