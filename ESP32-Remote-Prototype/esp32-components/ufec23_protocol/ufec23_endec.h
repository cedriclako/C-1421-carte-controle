#ifndef _UFEC23ENDEC_H_
#define _UFEC23ENDEC_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef enum
{
	UFEC23PROTOCOL_EVENTID_None = 0,
	UFEC23PROTOCOL_EVENTID_BootedUp = 1,
	UFEC23PROTOCOL_EVENTID_Test = 2,
} UFEC23PROTOCOL_EVENTID;

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

#define UFEC23ENDEC_SOFTWARENAME_LEN (12)
#define UFEC23ENDEC_GITHASH_LEN (12)

#define UFEC23ENDEC_PARAMETERITEM_KEY_LEN (48)

#define UFEC23ENDEC_VALUE_LEN (250)

#define UFEC23ENDEC_S2CREQPARAMETERGETRESPFLAGS_ISFIRSTRECORD (0x04)
#define UFEC23ENDEC_S2CREQPARAMETERGETRESPFLAGS_EOF (0x02)
#define UFEC23ENDEC_S2CREQPARAMETERGETRESPFLAGS_HASRECORD (0x01)

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
} UFEC23ENDEC_C2SGetParameter;

typedef enum
{
  UFEC23ENDEC_EPARAMTYPE_Int32 = 0,
  // UFEC23ENDEC_EPARAMTYPE_Float = 1
} UFEC23ENDEC_EPARAMTYPE;

typedef enum
{
	UFEC23ENDEC_EENTRYFLAGS_None = 0,
	UFEC23ENDEC_EENTRYFLAGS_Volatile = 1,
} UFEC23ENDEC_EENTRYFLAGS;

typedef union
{
  struct
  {
    int32_t s32Default;
    int32_t s32Min;
    int32_t s32Max;
  } sInt32;
  struct
  {
    float fltDefault;
    float fltMin;
    float fltMax;
  } sFloat;
} UFEC23ENDEC_uType;

typedef union
{
  int32_t s32Value;
  float fltValue;
} UFEC23ENDEC_uValue;

typedef struct 
{
  char szKey[UFEC23ENDEC_PARAMETERITEM_KEY_LEN+1];
  UFEC23ENDEC_EPARAMTYPE eParamType;
  UFEC23ENDEC_uType uType;
  UFEC23ENDEC_EENTRYFLAGS eEntryFlag;
} UFEC23ENDEC_SEntry;

typedef struct 
{
  UFEC23ENDEC_SEntry sEntry;
  UFEC23ENDEC_uValue uValue;
  bool bIsEOF;
  bool bHasRecord;
  bool bIsFirstRecord;
} UFEC23ENDEC_S2CReqParameterGetResp;

typedef struct
{
  char szKey[UFEC23ENDEC_PARAMETERITEM_KEY_LEN+1];
  UFEC23ENDEC_uValue uValue;
} UFEC23PROTOCOL_C2SSetParameter;

typedef struct
{
  uint32_t u32CRC32;
  uint32_t u32Size;
  UFEC23ENDEC_SVersion sVersion;
  uint16_t u16FirmwareID;
} UFEC23PROTOCOL_SServerFirmwareInfo;

typedef struct
{
  // Git
  char szGitCommitID[48+1];
  char szGitBranch[48+1];
  bool bGitIsDirty;
} UFEC23PROTOCOL_SServerGitInfo;

typedef enum
{
  UFEC23PROTOCOL_ERESULT_Ok = 0,
  UFEC23PROTOCOL_ERESULT_Fail = 1
} UFEC23PROTOCOL_ERESULT;

typedef struct 
{
  UFEC23PROTOCOL_ERESULT eResult;
} UFEC23PROTOCOL_S2CSetParameterResp;

// Maximum size for messages
#define UFEC23ENDEC_S2CREQPARAMETERGETRESP_MAX_COUNT (2 + 1 + 1 + sizeof(UFEC23ENDEC_uType)*3 + sizeof(UFEC23ENDEC_uValue) + 1 + UFEC23ENDEC_PARAMETERITEM_KEY_LEN + 1)

#define UFEC23ENDEC_S2CGETRUNNINGSETTINGRESP_COUNT (3)
#define UFEC23ENDEC_C2SSETRUNNINGSETTING_COUNT (4)

#define UFEC23ENDEC_C2SGETPARAMETER_COUNT (1)

// #define UFEC23ENDEC_C2SSETRUNNINGSETTING_COUNT (4)

#define UFEC23ENDEC_C2SSETPARAMETER_COUNT ( 1 + UFEC23ENDEC_PARAMETERITEM_KEY_LEN + 1 + sizeof(int32_t) )
#define UFEC23ENDEC_S2CSETPARAMETERRESP_COUNT (1)

#define UFEC23ENDEC_A2AREQPINGALIVE_COUNT (4)
#define UFEC23ENDEC_S2CREQPINGALIVERESP_COUNT (4)

#define UFEC23ENDEC_S2CEVENT_COUNT (1)

void UFEC23ENDEC_Init();

int32_t UFEC23ENDEC_C2SReqPingAliveEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23ENDEC_C2SReqPingAlive* pSrc);
bool UFEC23ENDEC_S2CReqPingAliveDecode(UFEC23ENDEC_C2SReqPingAlive* pDst, const uint8_t u8Datas[], uint32_t u32DataLen);

int32_t UFEC23ENDEC_S2CGetRunningSettingRespEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23ENDEC_S2CGetRunningSettingResp* pSrc);
bool UFEC23ENDEC_S2CGetRunningSettingRespDecode(UFEC23ENDEC_S2CGetRunningSettingResp* pDst, const uint8_t u8Datas[], uint32_t u32DataLen);

int32_t UFEC23ENDEC_C2SSetRunningSettingEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23ENDEC_C2SSetRunningSetting* pSrc);
bool UFEC23ENDEC_C2SSetRunningSettingDecode(UFEC23ENDEC_C2SSetRunningSetting* pDst, const uint8_t u8Datas[], uint32_t u32DataLen);

// ========================================
// Request one parameter
int32_t UFEC23ENDEC_C2SGetParameterEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23ENDEC_C2SGetParameter* pSrc);
bool UFEC23ENDEC_C2SGetParameterDecode(UFEC23ENDEC_C2SGetParameter* pDst, const uint8_t u8Datas[], uint32_t u32DataLen);


int32_t UFEC23ENDEC_S2CGetParameterRespEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23ENDEC_S2CReqParameterGetResp* pSrc);
bool UFEC23ENDEC_S2CGetParameterRespDecode(UFEC23ENDEC_S2CReqParameterGetResp* pDst, const uint8_t u8Datas[], uint32_t u32DataLen);


int32_t UFEC23ENDEC_C2SSetParameterEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23PROTOCOL_C2SSetParameter* pSrc);
bool UFEC23ENDEC_C2SSetParameterDecode(UFEC23PROTOCOL_C2SSetParameter* pDst, const uint8_t u8Datas[], uint32_t u32DataLen);

int32_t UFEC23ENDEC_S2CSetParameterRespEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23PROTOCOL_S2CSetParameterResp* pSrc);
bool UFEC23ENDEC_S2CSetParameterRespDecode(UFEC23PROTOCOL_S2CSetParameterResp* pDst, const uint8_t u8Datas[], uint32_t u32DataLen);

int32_t UFEC23ENDEC_S2CServerFirmwareInfoEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23PROTOCOL_SServerFirmwareInfo* pSrc);
bool UFEC23ENDEC_S2CServerFirmwareInfoRespDecode(UFEC23PROTOCOL_SServerFirmwareInfo* pDst, const uint8_t u8Datas[], uint32_t u32DataLen);

int32_t UFEC23ENDEC_S2CSendDebugDataRespEncode(uint8_t u8Dst[], uint32_t u32DstLen, const char* szJsonString);
bool UFEC23ENDEC_S2CSendDebugDataRespDecode(char szJsonStrings[], uint32_t u32JsonLen, const uint8_t u8Datas[], uint32_t u32DataLen);

int32_t UFEC23ENDEC_S2CSServerGitInfo(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23PROTOCOL_SServerGitInfo* pSrc);
bool UFEC23ENDEC_S2CServerGitInfoRespDecode(UFEC23PROTOCOL_SServerGitInfo* pDst, const uint8_t u8Datas[], uint32_t u32DataLen);

int32_t UFEC23ENDEC_S2CEncodeU16(uint8_t u8Dst[], uint32_t u32DstLen, uint16_t u16Value);

int32_t UFEC23ENDEC_S2CEncodeS32(uint8_t u8Dst[], uint32_t u32DstLen, int32_t s32Value);

bool UFEC23ENDEC_S2CDecodeS32(int32_t* ps32Value, const uint8_t u8Datas[], uint32_t u32DataLen);

// Event
int32_t UFEC23ENDEC_S2CEventEncode(uint8_t u8Dst[], uint32_t u32DstLen, UFEC23PROTOCOL_EVENTID eEventID);

#endif
