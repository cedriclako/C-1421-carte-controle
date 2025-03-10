#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "FreeRTOS/semphr.h"
#include "cJSON.h"
#include "stovemb.h"
#include "esp_log.h"

#define TAG "stovemb"

// JSON entries
#define JSON_ENTRIES_NAME "entries"

#define JSON_ENTRY_KEY_NAME "key"
#define JSON_ENTRY_VALUE_NAME "value"

#define JSON_ENTRY_INFO_DEFAULT_NAME "def"
#define JSON_ENTRY_INFO_MIN_NAME "min"
#define JSON_ENTRY_INFO_MAX_NAME "max"
#define JSON_ENTRY_INFO_TYPE_NAME "type"
#define JSON_ENTRY_INFO_ISVOLATILE "flag_isvolatile"

static STOVEMB_SMemBlock m_sMemBlock = {0};
static SemaphoreHandle_t m_xSemaphoreExt = NULL;

static STOVEMB_SParameterEntry* GetParamEntry(const char* szKey);

void STOVEMB_Init()
{
    m_xSemaphoreExt = xSemaphoreCreateRecursiveMutex();
    assert(m_xSemaphoreExt != NULL);

    STOVEMB_Reset();
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
    //ESP_LOGI(TAG, "STOVEMB_Take");
    return xSemaphoreTakeRecursive( m_xSemaphoreExt, xTicksToWait) == pdTRUE;
}

void STOVEMB_Give()
{
    //ESP_LOGI(TAG, "STOVEMB_Give");
    xSemaphoreGiveRecursive( m_xSemaphoreExt);
}

void STOVEMB_Reset()
{
    memset(&m_sMemBlock, 0, sizeof(STOVEMB_SMemBlock));
    
    // Default values ...
    m_sMemBlock.sRemoteData.bHasTempCurrentC = false;
    m_sMemBlock.sRemoteData.fTempCurrentC = 0.0f;

    // Remove data
    m_sMemBlock.sRemoteData.bHasTempSetPoint = true;
    m_sMemBlock.sRemoteData.sTempSetpoint.temp = 21.0f;
    m_sMemBlock.sRemoteData.sTempSetpoint.unit = SBI_iot_common_ETEMPERATUREUNIT_Celcius;

    // Fan speed
    m_sMemBlock.sRemoteData.bHasFanSpeed = true;
    m_sMemBlock.sRemoteData.eFanSpeedCurr = SBI_iot_common_EFANSPEED_Off;

    m_sMemBlock.sRemoteData.ttLastCommunicationTicks = 0;
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

    cJSON* pEntries = cJSON_AddArrayToObject(pRoot, JSON_ENTRIES_NAME);

    for(int32_t i = 0; i < m_sMemBlock.u32ParameterCount; i++)
    {
        cJSON* pEntryJSON = cJSON_CreateObject();
        if (pEntryJSON == NULL)
            goto ERROR;

        const STOVEMB_SParameterEntry* pEntryChanged = &m_sMemBlock.arrParameterEntries[i];

        cJSON_AddItemToObject(pEntryJSON, JSON_ENTRY_KEY_NAME, cJSON_CreateString(pEntryChanged->sEntry.szKey));
        
        if (pEntryChanged->sEntry.eParamType == UFEC23ENDEC_EPARAMTYPE_Int32)
        {
            cJSON_AddItemToObject(pEntryJSON, JSON_ENTRY_INFO_DEFAULT_NAME, cJSON_CreateNumber(pEntryChanged->sEntry.uType.sInt32.s32Default));
            cJSON_AddItemToObject(pEntryJSON, JSON_ENTRY_INFO_MIN_NAME, cJSON_CreateNumber(pEntryChanged->sEntry.uType.sInt32.s32Min));
            cJSON_AddItemToObject(pEntryJSON, JSON_ENTRY_INFO_MAX_NAME, cJSON_CreateNumber(pEntryChanged->sEntry.uType.sInt32.s32Max));
            cJSON_AddItemToObject(pEntryJSON, JSON_ENTRY_VALUE_NAME, cJSON_CreateNumber(pEntryChanged->sWriteValue.s32Value));
            cJSON_AddItemToObject(pEntryJSON, JSON_ENTRY_INFO_TYPE_NAME, cJSON_CreateString("int32"));
            const bool bIsVolatile = ((pEntryChanged->sEntry.eEntryFlag & UFEC23ENDEC_EENTRYFLAGS_Volatile) == UFEC23ENDEC_EENTRYFLAGS_Volatile);
            cJSON_AddItemToObject(pEntryJSON, JSON_ENTRY_INFO_ISVOLATILE, cJSON_CreateBool(bIsVolatile));
        }
        
        cJSON_AddItemToArray(pEntries, pEntryJSON);
    }

    szRet = cJSON_PrintUnformatted(pRoot);
    goto END;
    ERROR:
    ESP_LOGE(TAG, "STOVEMB_ExportParamToJSON error");
    END:
    if (pRoot != NULL)
        cJSON_Delete(pRoot);
    STOVEMB_Give();
    return szRet;
}

bool STOVEMB_InputParamFromJSON(const char* szJSON, char* szDstError, uint32_t u32DstErrorLen)
{
    bool bRet = true;
    cJSON* pRoot = NULL;
    char* szError = NULL;

    STOVEMB_Take(portMAX_DELAY);
    // Not ready
    if (!m_sMemBlock.bIsParameterDownloadCompleted)
    {
        szError = "Parameter list hasn't been downloaded yet";
        goto ERROR;
    }

    // Decode JSON
    pRoot = cJSON_Parse(szJSON);

    cJSON* pEntriesArray = cJSON_GetObjectItemCaseSensitive(pRoot, JSON_ENTRIES_NAME);
    if (!cJSON_IsArray(pEntriesArray))
    {
        szError = "Entries array is not valid";
        goto ERROR;
    }

    for(int i = 0; i < cJSON_GetArraySize(pEntriesArray); i++)
    {
        cJSON* pEntryJSON = cJSON_GetArrayItem(pEntriesArray, i);

        cJSON* pKeyJSON = cJSON_GetObjectItemCaseSensitive(pEntryJSON, JSON_ENTRY_KEY_NAME);
        if (pKeyJSON == NULL || !cJSON_IsString(pKeyJSON))
        {
            szError = "Cannot find JSON key element";
            goto ERROR;
        }

        cJSON* pValueJSON = cJSON_GetObjectItemCaseSensitive(pEntryJSON, JSON_ENTRY_VALUE_NAME);
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
            szError = "JSON value type is invalid, not a number";
            goto ERROR;
        }

        STOVEMB_SParameterEntry* pParamEntry = GetParamEntry(pKeyJSON->valuestring);
        if (pParamEntry == NULL)
        {
            ESP_LOGW(TAG, "parameter entry is not found : %s", pParamEntry->sEntry.szKey);
            continue;
        }

        // Buffer changes
        if (pParamEntry->sWriteValue.s32Value != (int32_t)pValueJSON->valueint)
        {
            ESP_LOGI(TAG, "Need write: '%s', value: %"PRId32" => %" PRId32, pParamEntry->sEntry.szKey, (int32_t)pParamEntry->sWriteValue.s32Value, (int32_t)pValueJSON->valueint);
            pParamEntry->sWriteValue.s32Value = (int32_t)pValueJSON->valueint;
            pParamEntry->bIsNeedWrite = true;
        }
    }

    bRet = true;
    ESP_LOGI(TAG, "Import JSON completed");
    goto END;
    ERROR:
    if (szError != NULL)
    {
        if (szDstError != NULL)
        {
            strncpy(szDstError, szError, u32DstErrorLen - 1);
            szDstError[u32DstErrorLen-1] = 0;
        }

        ESP_LOGE(TAG, "Error: %s", szError);
    }
    bRet = false;
    END:
    if (pRoot != NULL)
        cJSON_Delete(pRoot);
    STOVEMB_Give();
    return bRet;
}

int32_t STOVEMB_FindNextWritable(int32_t s32IndexStart, STOVEMB_SParameterEntry* pEntry)
{
    int32_t s32Ret = -1;
    STOVEMB_Take(portMAX_DELAY);
    // Find write
    for(int32_t i = s32IndexStart; i < m_sMemBlock.u32ParameterCount; i++)
    {
        const STOVEMB_SParameterEntry* pEntryParam = &m_sMemBlock.arrParameterEntries[i];
        if (pEntryParam->bIsNeedWrite)
        {
            *pEntry = *pEntryParam;
            s32Ret = i;
            break;
        }
    }
    STOVEMB_Give();
    return s32Ret;
}

STOVEMB_SParameterEntry* STOVEMB_GetByIndex(int32_t s32Index)
{
    STOVEMB_SParameterEntry* pEntryRet = NULL;
    STOVEMB_Take(portMAX_DELAY);

    // Not ready
    if (s32Index < 0 || s32Index >= m_sMemBlock.u32ParameterCount || !m_sMemBlock.bIsParameterDownloadCompleted)
    {
        goto END;
    }
    pEntryRet = &m_sMemBlock.arrParameterEntries[s32Index];
    END:
    STOVEMB_Give();
    return pEntryRet;
}

void STOVEMB_ResetAllParameterWriteFlag()
{
    STOVEMB_Take(portMAX_DELAY);
    // Find write
    for(int32_t i = 0; i < m_sMemBlock.u32ParameterCount; i++)
    {
        m_sMemBlock.arrParameterEntries[i].bIsNeedWrite = false;
    }
    STOVEMB_Give();
}

static STOVEMB_SParameterEntry* GetParamEntry(const char* szKey)
{
    STOVEMB_SParameterEntry* pEntryRet = NULL;
    STOVEMB_Take(portMAX_DELAY);

    // Not ready
    if (!m_sMemBlock.bIsParameterDownloadCompleted)
        goto END;

    for(int i = 0; i < m_sMemBlock.u32ParameterCount; i++)
    {
        STOVEMB_SParameterEntry* pEntry = &m_sMemBlock.arrParameterEntries[i];
        if (strcmp(pEntry->sEntry.szKey, szKey) == 0)
        {
            pEntryRet = pEntry;
            goto END;
        }
    }

    END:
    STOVEMB_Give();
    return pEntryRet;
}