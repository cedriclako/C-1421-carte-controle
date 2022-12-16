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
    int n = 0;
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
    // Version
    int n = 0;
    pDst->sVersion.u8Major = u8Datas[n++];
    pDst->sVersion.u8Minor = u8Datas[n++];
    pDst->sVersion.u8Revision = u8Datas[n++];
    const int swlen = u8Datas[n++];
    if (swlen > UFEC23ENDEC_SOFTWARENAME_LEN)
        return false;
    memcpy(pDst->szSoftwareName, u8Datas[n], swlen);
    pDst->szSoftwareName[swlen] = 0;
    n += swlen;
    const int gitHashLen = u8Datas[n++];
    if (gitHashLen > UFEC23ENDEC_GITHASH_LEN)
        return false;
    memcpy(pDst->szGitHash, u8Datas[n], gitHashLen);
    pDst->szGitHash[gitHashLen] = 0;
    n += gitHashLen;
    return true;
}
