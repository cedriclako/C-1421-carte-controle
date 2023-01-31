#include "SleepData.h"
#include "esp_partition.h"
#include "esp_spi_flash.h"
#include "esp_log.h"

#define TAG "SleepData"

#define PARTITION_SLEEPDATA_NAME "sleepdata"

static const uint8_t m_u8MagicNumbers[] = { 0x3c, 0x5c, 0xfc, 0xc6, 0xe4, 0xf8, 0x4a, 0x30, 
                                            0x8c, 0x25, 0x6c, 0x68, 0xef, 0x0b, 0xca, 0xFF };

// Needs to point to 4096 bytes block or bad thing will happens
//static const SLEEPDATA_URecord* m_pSleepDataRecords;

const esp_partition_t* m_pPartition = NULL;
static spi_flash_mmap_handle_t m_out_handle;

static int32_t m_s32LastRecordIndex = 0;

static void ResetRecordTable();

void SLEEPDATA_Init()
{
    m_pPartition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, PARTITION_SLEEPDATA_NAME);
    if (m_pPartition == NULL)
    {
        ESP_LOGE(TAG, "Partition not found: %s", PARTITION_SLEEPDATA_NAME);
        return;
    }

    // No valid magic number
    uint8_t u8Magics[SLEEPDATA_RECORD_SIZE];
    esp_partition_read(m_pPartition, 0, u8Magics, SLEEPDATA_RECORD_SIZE);
    
    if (memcmp(u8Magics, m_u8MagicNumbers, SLEEPDATA_RECORD_SIZE) != 0)
    {
        ESP_LOGE(TAG, "No record");

        // Write new header
        ResetRecordTable();
        
        // Index 0 is reserved for magic number so start at 1
        m_s32LastRecordIndex = 0;
    }
    else
    {
        // 0 = no record
        m_s32LastRecordIndex = 0;

        for(int32_t i = 1; i < SLEEPDATA_RECORDS_COUNT; i++)
        {
            SLEEPDATA_URecord sRecord;
            esp_partition_read(m_pPartition, i * SLEEPDATA_RECORD_SIZE, sRecord.u8Datas, SLEEPDATA_RECORD_SIZE);
            if (sRecord.sData.u8MagicByte == SLEEPDATA_RECORD_MAGICBYTE)
            {
                //ESP_LOGI(TAG, "[%d] Record is present, channel: %d", i, sRecord.sData.u8LastChannel);
                m_s32LastRecordIndex = i;
            }
        }

        ESP_LOGI(TAG, "Record is present, index: %d", m_s32LastRecordIndex);
    }
}

bool SLEEPDATA_ReadLastRecord(SLEEPDATA_URecord* pULastRecord)
{
    // 0 = no record
    if (m_pPartition == NULL || m_s32LastRecordIndex == 0)
        return false;

    esp_partition_read(m_pPartition, m_s32LastRecordIndex * SLEEPDATA_RECORD_SIZE, pULastRecord->u8Datas, SLEEPDATA_RECORD_SIZE);
    return true;
}


bool SLEEPDATA_WriteRecord(SLEEPDATA_URecord* pULastRecord)
{
    if (m_pPartition == NULL)
        return false;

    // No record?
    if (m_s32LastRecordIndex + 1 == SLEEPDATA_RECORDS_COUNT)
    {
        m_s32LastRecordIndex = 0;
        ResetRecordTable();
    }

    m_s32LastRecordIndex++;

    pULastRecord->sData.u8MagicByte = SLEEPDATA_RECORD_MAGICBYTE;
    ESP_ERROR_CHECK(esp_partition_write(m_pPartition, m_s32LastRecordIndex * SLEEPDATA_RECORD_SIZE, pULastRecord->u8Datas, SLEEPDATA_RECORD_SIZE));
 
    ESP_LOGI(TAG, "Write record is present, index: %d, offset: %d", m_s32LastRecordIndex, m_s32LastRecordIndex * SLEEPDATA_RECORD_SIZE);
    return true;
}

static void ResetRecordTable()
{ 
    ESP_ERROR_CHECK(esp_partition_erase_range(m_pPartition, 0, m_pPartition->size));
    ESP_ERROR_CHECK(esp_partition_write(m_pPartition, 0, m_u8MagicNumbers, SLEEPDATA_RECORD_SIZE));
}
