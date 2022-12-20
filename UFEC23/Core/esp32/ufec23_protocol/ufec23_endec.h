#ifndef _UFEC23ENDEC_H_
#define _UFEC23ENDEC_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef struct 
{
    uint8_t u8Major;
    uint8_t u8Minor;
    uint8_t u8Revision;
} UFEC23ENDEC_SVersion;

typedef struct 
{
    uint32_t u32Ping;
} UFEC23ENDEC_C2SReqPingAlive;

typedef struct 
{
    uint32_t u32Ping;
} UFEC23ENDEC_S2CReqPingAliveResp;

#define UFEC23ENDEC_SOFTWARENAME_LEN (12)
#define UFEC23ENDEC_GITHASH_LEN (12)

#define UFEC23ENDEC_PARAMETERITEM_KEY_LEN (15)

// Maximum size for messages
#define UFEC23ENDEC_S2CREQPARAMETERGETRESP_COUNT (2 + 1 + (4*4) + UFEC23ENDEC_PARAMETERITEM_KEY_LEN + 1)

#define UFEC23ENDEC_S2CREQPARAMETERGETRESPFLAGS_EOF (0x02)
#define UFEC23ENDEC_S2CREQPARAMETERGETRESPFLAGS_HASRECORD (0x01)

typedef struct 
{
    UFEC23ENDEC_SVersion sVersion;
    char szSoftwareName[UFEC23ENDEC_SOFTWARENAME_LEN+1];
    char szGitHash[UFEC23ENDEC_GITHASH_LEN+1];
} UFEC23ENDEC_S2CReqVersionResp;

typedef struct 
{
    uint8_t u8FanSpeedCurr;
    uint8_t u8FanSpeedMax;

    bool bIsAirOpen;
    bool bIsFanModeAuto;
} UFEC23ENDEC_S2CGetRunningSettingResp;

typedef enum 
{
  UFEC23ENDEC_EWRITESETTINGFLAGS_None = 0,
  UFEC23ENDEC_EWRITESETTINGFLAGS_FanSpeed = (1<<0),
  UFEC23ENDEC_EWRITESETTINGFLAGS_IsFanAutomatic = (1<<1),
  UFEC23ENDEC_EWRITESETTINGFLAGS_IsAirOpen = (1<<2),
} UFEC23ENDEC_EWRITESETTINGFLAGS;

typedef struct 
{
    UFEC23ENDEC_EWRITESETTINGFLAGS eRunningSettingFlags;
    uint8_t u8FanSpeedCurr;
    bool bIsFanModeAuto;
    bool bIsAirOpen;
} UFEC23ENDEC_C2SSetRunningSetting;

typedef enum
{
    UFEC23ENDEC_EITERATEOP_First = 0,	// Reset the iterator
    UFEC23ENDEC_EITERATEOP_Next = 1,	// Send next item

    UFEC23ENDEC_EITERATEOP_Count
} UFEC23ENDEC_EITERATEOP;

typedef struct 
{
    UFEC23ENDEC_EITERATEOP eIterateOp;
} UFEC23ENDEC_C2SReqParameterGet;

typedef enum
{
    UFEC23ENDEC_EPARAMTYPE_Int32 = 0,
    UFEC23ENDEC_EPARAMTYPE_Float = 1
} UFEC23ENDEC_EPARAMTYPE;


typedef union
{
    struct
    {
        int32_t s32Default;
        int32_t s32Min;
        int32_t s32Max;
        int32_t s32Value;
    } sInt32;
} UFEC23ENDEC_uType;

typedef struct 
{
    char szKey[UFEC23ENDEC_PARAMETERITEM_KEY_LEN+1];

    UFEC23ENDEC_EPARAMTYPE eParamType;
    UFEC23ENDEC_uType uType;
} UFEC23ENDEC_SEntry;


typedef struct 
{
    UFEC23ENDEC_SEntry sEntry;

    bool bIsEOF; 
	bool bHasRecord;
} UFEC23ENDEC_S2CReqParameterGetResp;

void UFEC23ENDEC_Init();

int32_t UFEC23ENDEC_S2CReqVersionRespEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23ENDEC_S2CReqVersionResp* pSrc);

bool UFEC23ENDEC_S2CReqVersionRespDecode(UFEC23ENDEC_S2CReqVersionResp* pDst, const uint8_t u8Datas[], uint32_t u32DataLen);

int32_t UFEC23ENDEC_S2CGetRunningSettingRespEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23ENDEC_S2CGetRunningSettingResp* pSrc);

bool UFEC23ENDEC_S2CGetRunningSettingRespDecode(UFEC23ENDEC_S2CGetRunningSettingResp* pDst, const uint8_t u8Datas[], uint32_t u32DataLen);

int32_t UFEC23ENDEC_C2SSetRunningSettingEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23ENDEC_C2SSetRunningSetting* pSrc);

bool UFEC23ENDEC_C2SSetRunningSettingDecode(UFEC23ENDEC_C2SSetRunningSetting* pDst, const uint8_t u8Datas[], uint32_t u32DataLen);

// ========================================
// Request one parameter
int32_t UFEC23ENDEC_C2SReqParameterGetEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23ENDEC_C2SReqParameterGet* pSrc);

bool UFEC23ENDEC_C2SReqParameterGetDecode(UFEC23ENDEC_C2SReqParameterGet* pDst, const uint8_t u8Datas[], uint32_t u32DataLen);


int32_t UFEC23ENDEC_S2CReqParameterGetRespEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23ENDEC_S2CReqParameterGetResp* pSrc);

bool UFEC23ENDEC_S2CReqParameterGetRespDecode(UFEC23ENDEC_S2CReqParameterGetResp* pDst, const uint8_t u8Datas[], uint32_t u32DataLen);


#endif
