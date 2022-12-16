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
	//PFL_TYPE_Float,
	PFL_TYPE_Int32
} PFL_TYPE;

typedef struct
{
	const char* szKey;
	const char* szDesc;
	PFL_TYPE eType;
	void* vdVar;
	union
	{
		/*struct
		{
			float fDefault;
			float fMin;
			float fMax;
		} sFloat;*/
		struct
		{
			int32_t s32Default;
			int32_t s32Min;
			int32_t s32Max;
		} sInt32;
	} uType;
} PFL_SParameterItem;

typedef struct _PFL_SHandle PFL_SHandle;

typedef void (*FnLoadAll)(const PFL_SHandle* psHandle);
typedef void (*FnCommitAll)(const PFL_SHandle* psHandle);

typedef struct
{
	FnLoadAll ptrLoadAll;
	FnCommitAll ptrCommitAll;
} PFL_SConfig;

struct _PFL_SHandle
{
	const PFL_SParameterItem* pParameterEntries;
	uint32_t u32ParameterEntryCount;

	const PFL_SConfig* psConfig;
};

typedef enum
{
    PFL_ESETRET_OK = 0,
    PFL_ESETRET_CannotSet = 1,
    PFL_ESETRET_InvalidRange = 2,
    PFL_ESETRET_ValidatorFailed = 3,
	PFL_ESETRET_EntryNoFound = 4,
} PFL_ESETRET;

#define PFL_INIT_SINT32(_key,_desc,_ptrvdvar,_defaultValue,_minValue,_maxValue) { .szKey = _key, .vdVar = _ptrvdvar,.szDesc = _desc, .eType = PFL_TYPE_Int32, .uType = { .sInt32 = { .s32Default = _defaultValue, .s32Min = _minValue, .s32Max = _maxValue} } }

void PFL_Init(PFL_SHandle* pHandle, const PFL_SParameterItem* pParameterEntries, uint32_t u32ParameterEntryCount, const PFL_SConfig* psConfig);

void PFL_LoadAll(PFL_SHandle* pHandle);

void PFL_CommitAll(PFL_SHandle* pHandle);

PFL_ESETRET PFL_GetValueInt32(const PFL_SHandle* pHandle, const char* szName, int32_t* psOut32Value);

PFL_ESETRET PFL_SetValueInt32(const PFL_SHandle* pHandle, const char* szName, int32_t s32NewValue);

int32_t PFL_ExportToJSON(const PFL_SHandle* pHandle, char* szBuffer, int32_t s32MaxLen);

bool PFL_ImportFromJSON(const PFL_SHandle* pHandle, const char* szBuffer, int32_t s32MaxLen);

#endif /* INC_PARAMETERFILE_H_ */
