#include "freertos/FreeRTOS.h"
#include "FreeRTOS/semphr.h"
#include "cJSON.h"
#include "stovemb.h"

static STOVEMB_SMemBlock m_sMemBlock = {0};
static SemaphoreHandle_t m_xSemaphoreExt = NULL;

void STOVEMB_Init()
{
    m_xSemaphoreExt = xSemaphoreCreateRecursiveMutex();
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
    return xSemaphoreTakeRecursive( m_xSemaphoreExt, xTicksToWait) == pdTRUE;
}

void STOVEMB_Give()
{
    xSemaphoreGiveRecursive( m_xSemaphoreExt);
}

char* STOVEMB_ExportParamToJSON()
{
    char* szRet = NULL;
    cJSON* pRoot = NULL;

    STOVEMB_Take(portMAX_DELAY);

    // Not ready
    if (!m_sMemBlock.bIsParameterDownloadCompleted)
        goto ERROR;

    pRoot = cJSON_CreateObject();
    if (pRoot == NULL)
        goto ERROR;

    cJSON* pEntries = cJSON_AddArrayToObject(pRoot, "entries");

    for(int32_t i = 0; i < m_sMemBlock.u32ParameterCount; i++)
    {
        cJSON* pEntryJSON = cJSON_CreateObject();
        if (pEntryJSON == NULL)
            goto ERROR;

        const UFEC23ENDEC_SEntry* pEntry = &m_sMemBlock.arrParameterEntries[i];

        cJSON_AddItemToObject(pEntryJSON, "key", cJSON_CreateString(pEntry->szKey));
        
        if (pEntry->eParamType == UFEC23ENDEC_EPARAMTYPE_Int32)
        {
            cJSON_AddItemToObject(pEntryJSON, "default", cJSON_CreateNumber(pEntry->uType.sInt32.s32Default));
            cJSON_AddItemToObject(pEntryJSON, "min", cJSON_CreateNumber(pEntry->uType.sInt32.s32Min));
            cJSON_AddItemToObject(pEntryJSON, "max", cJSON_CreateNumber(pEntry->uType.sInt32.s32Max));
            cJSON_AddItemToObject(pEntryJSON, "value", cJSON_CreateNumber(pEntry->uType.sInt32.s32Value));
        }
        
        cJSON_AddItemToArray(pEntries, pEntryJSON);
    }

    szRet = cJSON_PrintUnformatted(pRoot);
    goto END;
    ERROR:
    END:
    if (pRoot != NULL)
        cJSON_Delete(pRoot);
    STOVEMB_Give();
    return szRet;
}