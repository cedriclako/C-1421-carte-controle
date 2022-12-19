#include "stovemb.h"

static STOVEMB_SMemBlock m_sMemBlock = {0};

void STOVEMB_Init()
{
    memset(&m_sMemBlock, 0, sizeof(STOVEMB_SMemBlock));
}

STOVEMB_SMemBlock* STOVEMB_GetMemBlock()
{
    return &m_sMemBlock;
}

const STOVEMB_SMemBlock* STOVEMB_GetMemBlockRO()
{
    return &m_sMemBlock;
}