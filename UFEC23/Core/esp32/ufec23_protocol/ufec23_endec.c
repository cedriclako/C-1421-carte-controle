#include "ufec23_endec.h"

void UFEC23ENDEC_Init()
{
    
}

int32_t UFEC23ENDEC_S2CReqVersionRespEncode(uint8_t u8Dst[], uint32_t u32DstLen, const UFEC23ENDEC_S2CReqVersionResp* pSrc)
{
    const uint32_t u32SWNameLen = strlen(pSrc->szSoftwareName);
    if (u32SWNameLen > UFEC23ENDEC_SOFTWARENAME_LEN)
        return 0;
    const uint32_t u32GitHashLen = strlen(pSrc->szGitHash);
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
    u8Dst[n++] = (pSrc->bIsAirOpen ? 0x01 : 0x00) | (pSrc->bIsFanModeAuto ? 0x02 : 0x00);
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
    u8Dst[n++] = (u16 >> 8) & 0xFF;
    u8Dst[n++] = u16 & 0xFF;

    u8Dst[n++] = pSrc->u8FanSpeedCurr;
    u8Dst[n++] = (pSrc->bIsAirOpen ? 0x01 : 0x00) | (pSrc->bIsFanModeAuto ? 0x02 : 0x00);
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
