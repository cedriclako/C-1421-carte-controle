#include "freertos/FreeRTOS.h"
#include "FreeRTOS/semphr.h"

#include "stovemb.h"

static STOVEMB_SMemBlock m_sMemBlock = {0};
static SemaphoreHandle_t m_xSemaphoreExt = NULL;

void STOVEMB_Init()
{
    m_xSemaphoreExt = xSemaphoreCreateMutex();
    assert(m_xSemaphoreExt != NULL);

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

bool STOVEMB_Take(TickType_t xTicksToWait)
{
    return xSemaphoreTake( m_xSemaphoreExt, xTicksToWait) == pdTRUE;
}

void STOVEMB_Give()
{
    xSemaphoreGive( m_xSemaphoreExt);
}

char* STOVEMB_CopyServerParameterJSONTo()
{
    char* szCopy = NULL;
    STOVEMB_Take(portMAX_DELAY);
    if (m_sMemBlock.pS2CConfigJSON == NULL || m_sMemBlock.u32S2CConfigJSONLen == 0)
        goto ERROR;
    szCopy = malloc(sizeof(uint8_t) * m_sMemBlock.u32S2CConfigJSONLen);
    if (szCopy == NULL)
        goto ERROR;
    memcpy(szCopy, m_sMemBlock.pS2CConfigJSON, m_sMemBlock.u32S2CConfigJSONLen);
    goto END;
    ERROR:
    if (szCopy)
        free(szCopy);
    szCopy = NULL;
    END:
    STOVEMB_Give();
    return szCopy;
}