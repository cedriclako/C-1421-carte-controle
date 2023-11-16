#include "stm32-process.h"

static const char *TAG_STM_FLASH = "stm_flash";

static esp_err_t STM32PROCESS_WriteTask(const STM32PROTOCOL_SContext* pContext, FILE *flash_file);
static esp_err_t STM32PROCESS_ReadTask(const STM32PROTOCOL_SContext* pContext, FILE *flash_file);

static esp_err_t WriteTask(const STM32PROTOCOL_SContext* pContext, FILE *flash_file)
{
    ESP_LOGI(TAG_STM_FLASH, "%s", "Write Task");

    char loadAddress[4] = {0x08, 0x00, 0x00, 0x00};
    uint8_t block[256] = {0};
    int curr_block = 0, bytes_read = 0;

    fseek(flash_file, 0, SEEK_SET);
    if (STM32PROTOCOL_SetupSTM(pContext) != ESP_OK)
    {
        ESP_LOGE(TAG_STM_FLASH, "unable to setup the STM32");
        return ESP_FAIL;
    }

    while ((bytes_read = fread(block, 1, 256, flash_file)) > 0)
    {
        curr_block++;
        ESP_LOGI(TAG_STM_FLASH, "Writing block: %d", curr_block);
        // ESP_LOG_BUFFER_HEXDUMP("Block:  ", block, sizeof(block), ESP_LOG_DEBUG);
        const esp_err_t ret = STM32PROTOCOL_FlashPage(pContext, loadAddress, block);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG_STM_FLASH, "flash page process failed");
            return ESP_FAIL;
        }

        STM32PROTOCOL_IncrementLoadAddress(pContext, loadAddress);
        memset(block, 0xff, 256);
    }

    return ESP_OK;
}

static esp_err_t ReadTask(const STM32PROTOCOL_SContext* pContext, FILE *flash_file)
{
    ESP_LOGI(TAG_STM_FLASH, "Read & Verification Task");
    char readAddress[4] = {0x08, 0x00, 0x00, 0x00};

    uint8_t block[257] = {0};
    int curr_block = 0, bytes_read = 0;

    fseek(flash_file, 0, SEEK_SET);

    while ((bytes_read = fread(block, 1, 256, flash_file)) > 0)
    {
        curr_block++;
        ESP_LOGI(TAG_STM_FLASH, "Reading block: %d", curr_block);
        // ESP_LOG_BUFFER_HEXDUMP("Block:  ", block, sizeof(block), ESP_LOG_DEBUG);

        const esp_err_t ret = STM32PROTOCOL_ReadPage(pContext, readAddress, block);
        if (ret != ESP_OK)
        {
            return ESP_FAIL;
        }

        STM32PROTOCOL_IncrementLoadAddress(pContext, readAddress);
        memset(block, 0xff, 256);
    }

    return ESP_OK;
}

esp_err_t STM32PROCESS_FlashSTM(const STM32PROTOCOL_SContext* pContext, const char* filename)
{
    esp_err_t err = ESP_OK;

    // Initialize GPIO and UART
    STM32PROTOCOL_StartConn(pContext);

    ESP_LOGI(TAG_STM_FLASH, "File name: %s", filename);

    FILE* flash_file = fopen(filename, "rb");
    if (flash_file == NULL)
    {
        ESP_LOGE(TAG_STM_FLASH, "Cannot open file");
        goto ERROR;
    }

    // This while loop executes only once and breaks if any of the functions do not return ESP_OK
    ESP_LOGI(TAG_STM_FLASH, "Writing STM32 Memory");
    if (ESP_OK != (err = WriteTask(pContext, flash_file)))
    {
        ESP_LOGE(TAG_STM_FLASH, "Writing STM32 Memory process failed");
        goto ERROR;
    }
    ESP_LOGI(TAG_STM_FLASH, "Reading STM32 Memory");
    if (ESP_OK != (err = ReadTask(pContext, flash_file)))
    {
        ESP_LOGE(TAG_STM_FLASH, "Reading STM32 Memory process failed");
        goto ERROR;  
    }
    err = ESP_OK;
    ESP_LOGI(TAG_STM_FLASH, "STM32 Flashed Successfully!!!");
    goto END;
    ERROR:
    if (err == ESP_OK) {
        err = ESP_FAIL;
    }
    ESP_LOGI(TAG_STM_FLASH, "ERROR, unable to finish the process");
    END:
    ESP_LOGI(TAG_STM_FLASH, "Ending Connection");
    STM32PROTOCOL_EndConn(pContext);
    if (flash_file != NULL)
    {
        ESP_LOGI(TAG_STM_FLASH, "Closing file");
        fclose(flash_file);
    }
    return err;
}
