#include "freertos/FreeRTOS.h"
#include "FreeRTOS/semphr.h"
#include "cJSON.h"
#include "stovemb.h"
#include "esp_log.h"

#define TAG "stovemb"

static STOVEMB_SMemBlock m_sMemBlock = {0};
static SemaphoreHandle_t m_xSemaphoreExt = NULL;

static STOVEMB_SEntryChanged* GetParamEntry(const char* szKey);

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

bool STOVEMB_InputParamFromJSON(const char* szJSON)
{
    bool bRet = true;
    cJSON* pRoot = NULL;

    STOVEMB_Take(portMAX_DELAY);
    // Not ready
    if (!m_sMemBlock.bIsParameterDownloadCompleted)
        goto ERROR;

    // Decode JSON
    pRoot = cJSON_Parse(szJSON);

    cJSON* pEntriesArray = cJSON_GetObjectItemCaseSensitive(pRoot, "entries");
    if (!cJSON_IsArray(pEntriesArray))
    {
        ESP_LOGE(TAG, "Entries array is not valid");
        goto ERROR;
    }

    for(int i = 0; i < cJSON_GetArraySize(pEntriesArray); i++)
    {
        cJSON* pEntryJSON = cJSON_GetArrayItem(pEntriesArray, i);

        cJSON* pKeyJSON = cJSON_GetObjectItemCaseSensitive(pEntryJSON, "key");
        if (pKeyJSON == NULL || !cJSON_IsString(pKeyJSON))
        {
            ESP_LOGE(TAG, "Cannot find JSON key element");
            goto ERROR;
        }

        cJSON* pValueJSON = cJSON_GetObjectItemCaseSensitive(pEntryJSON, "value");
        if (pValueJSON == NULL)
        {
            // We just ignore changing the setting if the value property is not there.
            // it allows us to handle secret cases.
            ESP_LOGD(TAG, "JSON value is not there, skipping it");
            continue;
        }

        // We only support int32 for now.
        if (!cJSON_IsNumber(pValueJSON))
        {
            ESP_LOGE(TAG, "JSON value type is invalid, not a number");
            goto ERROR;
        }

        STOVEMB_SEntryChanged* pParamEntry = GetParamEntry(pKeyJSON->valuestring);
        if (pParamEntry == NULL)
        {
            ESP_LOGW(TAG, "parameter entry is not found : %s", pParamEntry->sEntry.szKey);
            continue;
        }

        // 
        pParamEntry->sWriteValue.s32Value = (int32_t)pValueJSON->valueint;
        pParamEntry->bIsNeedWrite = true;
    }

    bRet = true;
    ESP_LOGI(TAG, "Import JSON completed");
    goto END;
    ERROR:
    bRet = false;
    END:
    if (pRoot != NULL)
        cJSON_Delete(pRoot);
    STOVEMB_Give();
    return bRet;
}

static STOVEMB_SEntryChanged* GetParamEntry(const char* szKey)
{
    // Not ready
    if (!m_sMemBlock.bIsParameterDownloadCompleted)
        return NULL;
    
    for(int i = 0; i < m_sMemBlock.u32ParameterCount; i++)
    {
        STOVEMB_SEntryChanged* pEntry = &m_sMemBlock.arrParameterEntries[i];
        if (strcmp(pEntry->sEntry.szKey, szKey) == 0)
            return pEntry;
    }
    return NULL;
}
