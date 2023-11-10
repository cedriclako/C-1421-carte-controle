#include "ufec23_endec.h"
#include "ufec_stream.h"

void UFEC23ENDEC_Init()
{
    
}

int32_t UFEC23ENDEC_C2SReqPingAliveEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23ENDEC_C2SReqPingAlive* pSrc)
{
    if (u32DstLen < UFEC23ENDEC_A2AREQPINGALIVE_COUNT)
        return 0;
    memcpy(u8Dst, &pSrc->u32Ping, sizeof(uint32_t));
    return sizeof(uint32_t);
}

bool UFEC23ENDEC_S2CReqPingAliveDecode(UFEC23ENDEC_C2SReqPingAlive* pDst, const uint8_t u8Datas[], uint32_t u32DataLen)
{
    if (u32DataLen < UFEC23ENDEC_A2AREQPINGALIVE_COUNT)
        return false;
    memcpy(&pDst->u32Ping, u8Datas, sizeof(uint32_t));
    return true;
}

int32_t UFEC23ENDEC_S2CGetRunningSettingRespEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23ENDEC_S2CGetRunningSettingResp* pSrc)
{
 if (u32DstLen < UFEC23ENDEC_S2CGETRUNNINGSETTINGRESP_COUNT)
	 return 0;
 int n = 0;
 u8Dst[n++] = pSrc->u8FanSpeedCurr;
 u8Dst[n++] = pSrc->u8FanSpeedMax;
 u8Dst[n++] = (uint8_t)((pSrc->bIsAirOpen ? 0x01 : 0x00) | (pSrc->bIsFanModeAuto ? 0x02 : 0x00));
 return n;
}

bool UFEC23ENDEC_S2CGetRunningSettingRespDecode(UFEC23ENDEC_S2CGetRunningSettingResp* pDst, const uint8_t u8Datas[], uint32_t u32DataLen)
{
 if (u32DataLen < UFEC23ENDEC_S2CGETRUNNINGSETTINGRESP_COUNT)
	 return false;
 pDst->u8FanSpeedCurr = u8Datas[0];
 pDst->u8FanSpeedMax = u8Datas[1];
 pDst->bIsAirOpen = (u8Datas[2] & 0x01) != 0;
 pDst->bIsFanModeAuto = (u8Datas[2] & 0x02) != 0;
 return true;
}

int32_t UFEC23ENDEC_C2SSetRunningSettingEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23ENDEC_C2SSetRunningSetting* pSrc)
{
	if (u32DstLen < UFEC23ENDEC_C2SSETRUNNINGSETTING_COUNT)
		return 0;
	int n = 0;
	const uint16_t u16 = (uint16_t)pSrc->eRunningSettingFlags;
	u8Dst[n++] = (uint8_t)((u16 >> 8) & 0xFF);
	u8Dst[n++] = (uint8_t)(u16 & 0xFF);

	u8Dst[n++] = pSrc->u8FanSpeedCurr;
	u8Dst[n++] = (uint8_t)((pSrc->bIsAirOpen ? 0x01 : 0x00) | (pSrc->bIsFanModeAuto ? 0x02 : 0x00));
	return n;
}

int32_t UFEC23ENDEC_C2SGetParameterEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23ENDEC_C2SGetParameter* pSrc)
{
    if (u32DstLen < UFEC23ENDEC_C2SGETPARAMETER_COUNT)
        return false;
    int n = 0;
    u8Dst[n++] = (uint8_t)pSrc->eIterateOp;
    return n;
}

bool UFEC23ENDEC_C2SGetParameterDecode(UFEC23ENDEC_C2SGetParameter* pDst, const uint8_t u8Datas[], uint32_t u32DataLen)
{
    if (u32DataLen < UFEC23ENDEC_C2SGETPARAMETER_COUNT)
        return false;
    pDst->eIterateOp = (UFEC23ENDEC_EITERATEOP)u8Datas[0];
    if (pDst->eIterateOp >= UFEC23ENDEC_EITERATEOP_Count)
        return false;
    return true;
}

/*
bool UFEC23ENDEC_C2SSetRunningSettingDecode(UFEC23ENDEC_C2SSetRunningSetting* pDst, const uint8_t u8Datas[], uint32_t u32DataLen)
{
    if (u32DataLen < UFEC23ENDEC_C2SSETRUNNINGSETTING_COUNT)
        return false;
    const uint16_t u16 = (uint16_t)((u8Datas[0] << 8) | u8Datas[1]);
    pDst->eRunningSettingFlags = (UFEC23ENDEC_EWRITESETTINGFLAGS)u16;
    pDst->u8FanSpeedCurr = u8Datas[2];
    pDst->bIsAirOpen = (u8Datas[3] & 0x01) ? 0x01 : 0x00;
    pDst->bIsFanModeAuto = (u8Datas[3] & 0x02) ? 0x01 : 0x00;
    return true;
}*/

int32_t UFEC23ENDEC_S2CGetParameterRespEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23ENDEC_S2CReqParameterGetResp* pSrc)
{
    if (u32DstLen < UFEC23ENDEC_S2CREQPARAMETERGETRESP_MAX_COUNT)
        return 0;
 
    const UFEC23ENDEC_SEntry* psEntry = &pSrc->sEntry;
	int32_t n = 0;
	u8Dst[n++] = (uint8_t)((pSrc->bHasRecord ? (uint8_t)UFEC23ENDEC_S2CREQPARAMETERGETRESPFLAGS_HASRECORD : 0x00) |
                           (pSrc->bIsEOF ? (uint8_t)UFEC23ENDEC_S2CREQPARAMETERGETRESPFLAGS_EOF : 0x00)) |
                           (pSrc->bIsFirstRecord ? (uint8_t)UFEC23ENDEC_S2CREQPARAMETERGETRESPFLAGS_ISFIRSTRECORD : 0x00);
	u8Dst[n++] = (uint8_t)psEntry->eParamType;
	u8Dst[n++] = (uint8_t)psEntry->eEntryFlag;
	const uint8_t u8KeyLen = (uint8_t)strnlen(psEntry->szKey, UFEC23ENDEC_PARAMETERITEM_KEY_LEN+1);
	if (u8KeyLen > UFEC23ENDEC_PARAMETERITEM_KEY_LEN)
		return 0;
	u8Dst[n++] = (uint8_t)u8KeyLen;
    memcpy(u8Dst + n, psEntry->szKey, (size_t)u8KeyLen);
    n += u8KeyLen;
    if (psEntry->eParamType == UFEC23ENDEC_EPARAMTYPE_Int32)
    {
        memcpy(&u8Dst[n], &psEntry->uType.sInt32.s32Default, sizeof(int32_t));
        n += sizeof(int32_t);
        memcpy(&u8Dst[n], &psEntry->uType.sInt32.s32Min, sizeof(int32_t));
        n += sizeof(int32_t);
        memcpy(&u8Dst[n], &psEntry->uType.sInt32.s32Max, sizeof(int32_t));
        n += sizeof(int32_t);
        memcpy(&u8Dst[n], &pSrc->uValue.s32Value, sizeof(int32_t));
        n += sizeof(int32_t);
    }
    else
    {
        // Not supported
        return 0;
    }
	return n;
}

bool UFEC23ENDEC_S2CGetParameterRespDecode(UFEC23ENDEC_S2CReqParameterGetResp* pDst, const uint8_t u8Datas[], uint32_t u32DataLen)
{
    if (u32DataLen < 3)
        return false;

    uint32_t n = 0;
    pDst->bHasRecord = (u8Datas[n] & UFEC23ENDEC_S2CREQPARAMETERGETRESPFLAGS_HASRECORD) != 0;
    pDst->bIsEOF = (u8Datas[n] & UFEC23ENDEC_S2CREQPARAMETERGETRESPFLAGS_EOF) != 0; // Flags
    pDst->bIsFirstRecord = (u8Datas[n] & UFEC23ENDEC_S2CREQPARAMETERGETRESPFLAGS_ISFIRSTRECORD) != 0; // Flags
    n++;
    pDst->sEntry.eParamType = (UFEC23ENDEC_EPARAMTYPE)u8Datas[n++];
    pDst->sEntry.eEntryFlag = (UFEC23ENDEC_EENTRYFLAGS)u8Datas[n++];
    const uint8_t u8KeyLen = u8Datas[n++];
    if (u8KeyLen > UFEC23ENDEC_PARAMETERITEM_KEY_LEN)
        return false;

    if (u32DataLen < (3 + u8KeyLen + sizeof(int32_t) * 4))
        return false;

    memcpy(pDst->sEntry.szKey, &u8Datas[n], u8KeyLen);
    pDst->sEntry.szKey[u8KeyLen] = 0;
    n += u8KeyLen;
    if (pDst->sEntry.eParamType == UFEC23ENDEC_EPARAMTYPE_Int32)
    {
        memcpy(&pDst->sEntry.uType.sInt32.s32Default, &u8Datas[n], sizeof(int32_t));
        n += sizeof(int32_t);
        memcpy(&pDst->sEntry.uType.sInt32.s32Min, &u8Datas[n], sizeof(int32_t));
        n += sizeof(int32_t);
        memcpy(&pDst->sEntry.uType.sInt32.s32Max, &u8Datas[n], sizeof(int32_t));
        n += sizeof(int32_t);
        memcpy(&pDst->uValue.s32Value, &u8Datas[n], sizeof(int32_t));
        n += sizeof(int32_t);
    }
    else
    {
        return false;
    }

	return true;
}

int32_t UFEC23ENDEC_C2SSetParameterEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23PROTOCOL_C2SSetParameter* pSrc)
{
    if (u32DstLen < UFEC23ENDEC_C2SSETPARAMETER_COUNT)
        return false;
    int n = 0;
	const uint8_t u8KeyLen = (uint8_t)strnlen(pSrc->szKey, UFEC23ENDEC_PARAMETERITEM_KEY_LEN+1);
	if (u8KeyLen > UFEC23ENDEC_PARAMETERITEM_KEY_LEN)
		return 0;
    u8Dst[n++] = u8KeyLen;
    memcpy(&u8Dst[n], pSrc->szKey, u8KeyLen);
    n += u8KeyLen;
    memcpy(&u8Dst[n], &pSrc->uValue, sizeof(UFEC23ENDEC_uValue));
    n += sizeof(UFEC23ENDEC_uValue);
    return n;
}

bool UFEC23ENDEC_C2SSetParameterDecode(UFEC23PROTOCOL_C2SSetParameter* pDst, const uint8_t u8Datas[], uint32_t u32DataLen)
{
    if (u32DataLen < 1)
        return false;
    int32_t n = 0;
    const uint8_t u8KeyLen = u8Datas[n++];
    if (u32DataLen < 1 + u8KeyLen)
        return false;
    memcpy(pDst->szKey, &u8Datas[n], u8KeyLen);
    pDst->szKey[u8KeyLen] = 0;
    n += u8KeyLen;
    if (u32DataLen < 1 + u8KeyLen + sizeof(UFEC23ENDEC_uValue))
        return false;
    memcpy(&pDst->uValue, &u8Datas[n], sizeof(UFEC23ENDEC_uValue));
    n += sizeof(UFEC23ENDEC_uValue);
    return true;
}

int32_t UFEC23ENDEC_S2CSetParameterRespEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23PROTOCOL_S2CSetParameterResp* pSrc)
{
    if (u32DstLen < UFEC23ENDEC_S2CSETPARAMETERRESP_COUNT)
        return 0;
    int32_t n = 0;
    u8Dst[n++] = (uint8_t)pSrc->eResult;
    return n;
}

bool UFEC23ENDEC_S2CSetParameterRespDecode(UFEC23PROTOCOL_S2CSetParameterResp* pDst, const uint8_t u8Datas[], uint32_t u32DataLen)
{
    if (u32DataLen < UFEC23ENDEC_S2CSETPARAMETERRESP_COUNT)
        return 0;
    pDst->eResult = (UFEC23PROTOCOL_ERESULT)u8Datas[0];
    return true;
}

int32_t UFEC23ENDEC_S2CEventEncode(uint8_t u8Dst[], uint32_t u32DstLen, UFEC23PROTOCOL_EVENTID eEventID)
{
    if (u32DstLen < UFEC23ENDEC_S2CEVENT_COUNT)
        return 0;
    int32_t n = 0;
    u8Dst[n++] = (uint8_t)eEventID;
    return n;
}

int32_t UFEC23ENDEC_S2CSendDebugDataRespEncode(uint8_t u8Dst[], uint32_t u32DstLen, const char* szJsonString)
{
	const size_t len = strlen(szJsonString);
	if ((len + sizeof(uint16_t)) > u32DstLen)
		return 0;
	const uint16_t u16StringLen = len;
	memcpy((char*)u8Dst, &u16StringLen, sizeof(uint16_t));
	int32_t n = sizeof(int16_t);
	memcpy((char*)u8Dst + n, szJsonString, len);
	n += len;
	return n;
}

bool UFEC23ENDEC_S2CSendDebugDataRespDecode(char szJsonStrings[], uint32_t u32JsonLen, const uint8_t u8Datas[], uint32_t u32DataLen)
{
    uint16_t u16Len = 0;
    int32_t n = 0;
    if (u32DataLen < sizeof(uint16_t)) 
        return false;
    memcpy(&u16Len, u8Datas, sizeof(uint16_t));
    n += sizeof(uint16_t); // Feel the earth move, and then ...
    if (n + u16Len > u32DataLen || u16Len + 1 > u32JsonLen) 
        return false;
    strncpy( (char*)szJsonStrings, (const char*)(u8Datas+n), u32JsonLen);
    n += u16Len;
    szJsonStrings[u16Len] = '\0';
    return true;
}

int32_t UFEC23ENDEC_S2CEncodeU16(uint8_t u8Dst[], uint32_t u32DstLen, uint16_t u16Value)
{
	if (u32DstLen < sizeof(uint16_t))
		return false;
    memcpy(u8Dst, &u16Value, sizeof(uint16_t));
	return sizeof(uint16_t);
}

int32_t UFEC23ENDEC_S2CEncodeS32(uint8_t u8Dst[], uint32_t u32DstLen, int32_t s32Value)
{
	if (u32DstLen < sizeof(int32_t))
		return false;
    memcpy(u8Dst, &s32Value, sizeof(int32_t));
	return sizeof(int32_t);
}

bool UFEC23ENDEC_S2CDecodeS32(int32_t* ps32Value, const uint8_t u8Datas[], uint32_t u32DataLen)
{
    if (u32DataLen < sizeof(int32_t))
        return 0;
    memcpy(ps32Value, u8Datas, sizeof(int32_t));
    return true;
}

int32_t UFEC23ENDEC_S2CServerFirmwareInfoEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23PROTOCOL_SServerFirmwareInfo* pSrc)
{
  UFECSTREAM_SContext sContext;
  UFECSTREAM_Init(&sContext, u8Dst, u32DstLen);
  UFECSTREAM_WriteUInt32LE(&sContext, pSrc->u32CRC32);
  UFECSTREAM_WriteUInt32LE(&sContext, pSrc->u32Size);
  UFECSTREAM_WriteUInt16LE(&sContext, pSrc->u16FirmwareID);
  UFECSTREAM_WriteUInt8LE(&sContext, pSrc->sVersion.u8Major);
  UFECSTREAM_WriteUInt8LE(&sContext, pSrc->sVersion.u8Minor);
  UFECSTREAM_WriteUInt8LE(&sContext, pSrc->sVersion.u8Revision);
  return UFECSTREAM_Count(&sContext);
}

bool UFEC23ENDEC_S2CServerFirmwareInfoRespDecode(UFEC23PROTOCOL_SServerFirmwareInfo* pDst, const uint8_t u8Datas[], uint32_t u32DataLen)
{
  UFECSTREAM_SContext sContext;
  UFECSTREAM_Init(&sContext, (uint8_t*)u8Datas, u32DataLen);
  return UFECSTREAM_ReadUInt32LE(&sContext, &pDst->u32CRC32) &&
         UFECSTREAM_ReadUInt32LE(&sContext, &pDst->u32Size) &&
         UFECSTREAM_ReadUInt16LE(&sContext, &pDst->u16FirmwareID) &&
         UFECSTREAM_ReadUInt8LE(&sContext, &pDst->sVersion.u8Major) &&
         UFECSTREAM_ReadUInt8LE(&sContext, &pDst->sVersion.u8Minor) &&
         UFECSTREAM_ReadUInt8LE(&sContext, &pDst->sVersion.u8Revision);
}

int32_t UFEC23ENDEC_S2CSServerGitInfo(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23PROTOCOL_SServerGitInfo* pSrc)
{
  UFECSTREAM_SContext sContext;
  UFECSTREAM_Init(&sContext, u8Dst, u32DstLen);
  UFECSTREAM_WriteString(&sContext, pSrc->szGitCommitID);
  UFECSTREAM_WriteString(&sContext, pSrc->szGitBranch);
  return UFECSTREAM_Count(&sContext);
}

bool UFEC23ENDEC_S2CServerGitInfoRespDecode(UFEC23PROTOCOL_SServerGitInfo* pDst, const uint8_t u8Datas[], uint32_t u32DataLen)
{
  UFECSTREAM_SContext sContext;
  UFECSTREAM_Init(&sContext, (uint8_t*)u8Datas, u32DataLen);
  return UFECSTREAM_ReadString(&sContext, pDst->szGitCommitID, sizeof(pDst->szGitCommitID)/sizeof(pDst->szGitCommitID[0])) &&
         UFECSTREAM_ReadString(&sContext, pDst->szGitBranch, sizeof(pDst->szGitBranch)/sizeof(pDst->szGitBranch[0]));
}
