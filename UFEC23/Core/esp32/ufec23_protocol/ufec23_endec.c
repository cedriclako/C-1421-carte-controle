#include "ufec23_endec.h"

void UFEC23ENDEC_Init()
{
    
}

int32_t UFEC23ENDEC_S2CReqVersionRespEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23ENDEC_S2CReqVersionResp* pSrc)
{
    const uint8_t u32SWNameLen = (uint8_t)strlen(pSrc->szSoftwareName);
    if (u32SWNameLen > UFEC23ENDEC_SOFTWARENAME_LEN)
        return 0;
    const uint8_t u32GitHashLen = (uint8_t)strlen(pSrc->szGitHash);
    if (u32GitHashLen > UFEC23ENDEC_GITHASH_LEN)
        return 0;
    int32_t n = 0;
    u8Dst[n++] = pSrc->sVersion.u8Major;
    u8Dst[n++] = pSrc->sVersion.u8Minor;
    u8Dst[n++] = pSrc->sVersion.u8Revision;

    u8Dst[n++] = (uint8_t)u32SWNameLen;
    memcpy(u8Dst+n, pSrc->szSoftwareName, u32SWNameLen);
    n += u32SWNameLen;
    u8Dst[n++] = (uint8_t)u32GitHashLen;
    memcpy(u8Dst+n, pSrc->szGitHash, u32GitHashLen);
    n += u32GitHashLen;
    return n;
}

bool UFEC23ENDEC_S2CReqVersionRespDecode(UFEC23ENDEC_S2CReqVersionResp* pDst, const uint8_t u8Datas[], uint32_t u32DataLen)
{
    // 3 bytes for version + 1 for len + 1 for len
    if (u32DataLen < 5)
        return false;
    // Version
	uint32_t n = 0;
    pDst->sVersion.u8Major = u8Datas[n++];
    pDst->sVersion.u8Minor = u8Datas[n++];
    pDst->sVersion.u8Revision = u8Datas[n++];
    const uint32_t swlen = u8Datas[n++];
    if (swlen > UFEC23ENDEC_SOFTWARENAME_LEN || n + swlen > u32DataLen)
        return false;
    memcpy(pDst->szSoftwareName, &u8Datas[n], (size_t)swlen);
    pDst->szSoftwareName[swlen] = 0;
    n += swlen;
    const uint32_t gitHashLen = u8Datas[n++];
    if (gitHashLen > UFEC23ENDEC_GITHASH_LEN || n + gitHashLen > u32DataLen)
        return false;
    memcpy(pDst->szGitHash, &u8Datas[n], (size_t)gitHashLen);
    pDst->szGitHash[gitHashLen] = 0;
    n += gitHashLen;
    return true;
}

int32_t UFEC23ENDEC_S2CGetRunningSettingRespEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23ENDEC_S2CGetRunningSettingResp* pSrc)
{
    if (u32DstLen < 3)
        return 0;
    int n = 0;
    u8Dst[n++] = pSrc->u8FanSpeedCurr;
    u8Dst[n++] = pSrc->u8FanSpeedMax;
    u8Dst[n++] = (uint8_t)((pSrc->bIsAirOpen ? 0x01 : 0x00) | (pSrc->bIsFanModeAuto ? 0x02 : 0x00));
    return n;
}

bool UFEC23ENDEC_S2CGetRunningSettingRespDecode(UFEC23ENDEC_S2CGetRunningSettingResp* pDst, const uint8_t u8Datas[], uint32_t u32DataLen)
{
    if (u32DataLen < 3)
        return false;
    pDst->u8FanSpeedCurr = u8Datas[0];
    pDst->u8FanSpeedMax = u8Datas[1];
    pDst->bIsAirOpen = (u8Datas[2] & 0x01) ? true : false;
    pDst->bIsFanModeAuto = (u8Datas[2] & 0x02) ? true : false;
    return true;
}

int32_t UFEC23ENDEC_C2SSetRunningSettingEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23ENDEC_C2SSetRunningSetting* pSrc)
{
    if (u32DstLen < 4)
        return 0;
    int n = 0;
    const uint16_t u16 = (uint16_t)pSrc->eRunningSettingFlags;
    u8Dst[n++] = (uint8_t)((u16 >> 8) & 0xFF);
    u8Dst[n++] = (uint8_t)(u16 & 0xFF);

    u8Dst[n++] = pSrc->u8FanSpeedCurr;
    u8Dst[n++] = (uint8_t)((pSrc->bIsAirOpen ? 0x01 : 0x00) | (pSrc->bIsFanModeAuto ? 0x02 : 0x00));
    return n;
}

bool UFEC23ENDEC_C2SSetRunningSettingDecode(UFEC23ENDEC_C2SSetRunningSetting* pDst, const uint8_t u8Datas[], uint32_t u32DataLen)
{
    if (u32DataLen < 4)
        return 0;
    const uint16_t u16 = (uint16_t)((u8Datas[0] << 8) | u8Datas[1]);
    pDst->eRunningSettingFlags = (UFEC23ENDEC_EWRITESETTINGFLAGS)u16;
    pDst->u8FanSpeedCurr = u8Datas[2];
    pDst->bIsAirOpen = (u8Datas[3] & 0x01) ? 0x01 : 0x00;
    pDst->bIsFanModeAuto = (u8Datas[3] & 0x02) ? 0x01 : 0x00;
    return false;
}

int32_t UFEC23ENDEC_S2CReqParameterGetRespEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23ENDEC_S2CReqParameterGetResp* pSrc)
{
    const int32_t s32MinLen = 2 + 1 + (4*3) + UFEC23ENDEC_PARAMETERITEM_KEY_LEN + 1;
    if (u32DstLen < s32MinLen)
    {
        return 0;
    }

    const UFEC23ENDEC_SEntry* psEntry = &pSrc->sEntry;
    
	int32_t n = 0;
	u8Dst[n++] = (uint8_t)((pSrc->bHasRecord ? 0x01 : 0x00) | (pSrc->bIsEOF ? 0x02 : 0x00));
	u8Dst[n++] = (uint8_t)psEntry->eParamType;
	const uint8_t u8KeyLen = (uint8_t)strnlen(psEntry->szKey, UFEC23ENDEC_PARAMETERITEM_KEY_LEN+1);
	if (u8KeyLen > UFEC23ENDEC_PARAMETERITEM_KEY_LEN)
		return 0;
	u8Dst[n++] = (uint8_t)u8KeyLen;
    memcpy(u8Dst + n, psEntry->szKey, (size_t)u8KeyLen);
    n += u8KeyLen;

    if (psEntry->eParamType == UFEC23ENDEC_EPARAMTYPE_Int32)
    {
    	u8Dst[n] = (uint8_t)psEntry->uType.sInt32.s32Default;
        memcpy(&u8Dst[n], &psEntry->uType.sInt32.s32Default, sizeof(int32_t));
        n += sizeof(int32_t);
    	u8Dst[n] = (uint8_t)psEntry->uType.sInt32.s32Min;
        memcpy(&u8Dst[n], &psEntry->uType.sInt32.s32Min, sizeof(int32_t));
        n += sizeof(int32_t);
    	u8Dst[n] = (uint8_t)psEntry->uType.sInt32.s32Max;
        memcpy(&u8Dst[n], &psEntry->uType.sInt32.s32Max, sizeof(int32_t));
        n += sizeof(int32_t);
    }

	return n;
}

bool UFEC23ENDEC_S2CReqParameterGetRespDecode(UFEC23ENDEC_S2CReqParameterGetResp* pDst, const uint8_t u8Datas[], uint32_t u32DataLen)
{
	return true;
}

