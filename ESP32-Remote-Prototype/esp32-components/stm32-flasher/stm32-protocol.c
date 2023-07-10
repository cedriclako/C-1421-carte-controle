#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "stm32-protocol.h"
/*
    This is a crude version of the STM32 flash protocol.
    The original code works but only do the absolute minimum. 
    Not garanted to work in production.

    url: https://github.com/ESP32-Musings/OTA_update_STM32_using_ESP32
*/
#define TAG_STM_PRO "STMPROTOCOL"
#define LOW (0)
#define HIGH (1)

#define UART_BUF_SIZE (1024)

#define STM32PROTOCOL_ASSERTCONTEXT(_pContext) \
do \
{ \
    assert(_pContext != NULL); \
    assert(_pContext->psConfig != NULL); \
    \
    assert(_pContext->psConfig->reset_pin >= 0); \
    assert(_pContext->psConfig->boot0_pin >= 0); \
    assert(_pContext->psConfig->uart_port >= UART_NUM_0 && _pContext->psConfig->uart_port < UART_NUM_MAX ); \
    if (_pContext->psConfig->bInitUART) \
    { \
        assert(_pContext->psConfig->uart_tx_pin >= 0); \
        assert(_pContext->psConfig->uart_rx_pin >= 0); \
    } \
    assert(_pContext->psConfig->u32RecvDataTimeoutMS > 0); \
    assert(_pContext->psConfig->u32SyncDataTimeoutMS > 0); \
} \
while(0); \

#define STM32PROTOCOL_RETURNNOTOK(__ret) \
do \
{ \
    esp_err_t err22; \
    err22 = (__ret); \
    if (err22 != ESP_OK) \
    {\
        return err22; \
    } \
} while(0); \


static void InitGPIO(const STM32PROTOCOL_SContext* pContext);
static void InitFlashUART(const STM32PROTOCOL_SContext* pContext);

static int WaitForSerialData(const STM32PROTOCOL_SContext* pContext, int dataCount, int timeout);
static int SendData(const STM32PROTOCOL_SContext* pContext, const uint8_t* u8OutgoingData, const int count);
static esp_err_t SendReceiveBytes(const STM32PROTOCOL_SContext* pContext, const uint8_t* u8SendData, uint32_t u32SendCount, uint8_t* u8RecvData, uint32_t u32ExpectedRecvCount);

void STM32PROTOCOL_Init(STM32PROTOCOL_SContext* pContext, const STM32PROTOCOL_SConfig* pConfig)
{
	ESP_LOGI(TAG_STM_PRO, "Init STM32 flasher");
	pContext->psConfig = pConfig;
    STM32PROTOCOL_ASSERTCONTEXT(pContext);

	if (pConfig->bInitGPIO)
		InitGPIO(pContext);
	
	if (pConfig->bInitUART)
		InitFlashUART(pContext);
}

esp_err_t STM32PROTOCOL_SetupSTM(const STM32PROTOCOL_SContext* pContext)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    
    STM32PROTOCOL_RETURNNOTOK(STM32PROTOCOL_ResetSTM(pContext, true));
    STM32PROTOCOL_RETURNNOTOK(STM32PROTOCOL_CmdSync(pContext));
    STM32PROTOCOL_RETURNNOTOK(STM32PROTOCOL_CmdGet(pContext));
    STM32PROTOCOL_RETURNNOTOK(STM32PROTOCOL_CmdVersion(pContext));
    STM32PROTOCOL_RETURNNOTOK(STM32PROTOCOL_CmdId(pContext));
    STM32PROTOCOL_RETURNNOTOK(STM32PROTOCOL_CmdErase(pContext));
    STM32PROTOCOL_RETURNNOTOK(STM32PROTOCOL_CmdExtErase(pContext));
	return ESP_OK;
}

esp_err_t STM32PROTOCOL_EndConn(const STM32PROTOCOL_SContext* pContext)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    STM32PROTOCOL_RETURNNOTOK(STM32PROTOCOL_ResetSTM(pContext, false));

    ESP_LOGI(TAG_STM_PRO, "Ending Connection");
	return ESP_OK;
}

esp_err_t STM32PROTOCOL_ResetSTM(const STM32PROTOCOL_SContext* pContext, bool bIsBootloaderMode)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
	ESP_LOGI(TAG_STM_PRO, "Starting RESET Procedure");
    // According to the datasheet, putting BOOTPIN0 to HIGH will boot the system in bootloader mode.
    gpio_set_level(pContext->psConfig->boot0_pin, (bIsBootloaderMode ? HIGH : LOW));
	gpio_set_level(pContext->psConfig->reset_pin, LOW);
	vTaskDelay(pdMS_TO_TICKS(100));
	gpio_set_level(pContext->psConfig->reset_pin, HIGH);
	vTaskDelay(pdMS_TO_TICKS(500));
	ESP_LOGI(TAG_STM_PRO, "Finished RESET Procedure");
    return ESP_OK;
}

esp_err_t STM32PROTOCOL_CmdSync(const STM32PROTOCOL_SContext* pContext)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    ESP_LOGI(TAG_STM_PRO, "SYNC");
    const uint8_t bytes[] = { 0x7F };
    
    const uint32_t u32RespLen = 1;
    uint8_t u8RespData[1];
    return SendReceiveBytes(pContext, bytes, sizeof(bytes), u8RespData, u32RespLen);
}

esp_err_t STM32PROTOCOL_CmdGet(const STM32PROTOCOL_SContext* pContext)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    ESP_LOGI(TAG_STM_PRO, "GET");
    const uint8_t bytes[] = {0x00, 0xFF};

    const int u32RespLen = 15;
    uint8_t u8RespData[15];
    return SendReceiveBytes(pContext, bytes, sizeof(bytes), u8RespData, u32RespLen);
}

esp_err_t STM32PROTOCOL_CmdVersion(const STM32PROTOCOL_SContext* pContext)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    ESP_LOGI(TAG_STM_PRO, "GET VERSION & READ PROTECTION STATUS");
    const uint8_t  bytes[] = {0x01, 0xFE};

    const int u32RespLen = 5;
    uint8_t u8RespData[5];
    return SendReceiveBytes(pContext, bytes, sizeof(bytes), u8RespData, u32RespLen);
}

esp_err_t STM32PROTOCOL_CmdId(const STM32PROTOCOL_SContext* pContext)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    ESP_LOGI(TAG_STM_PRO, "CHECK ID");
    const uint8_t bytes[] = {0x02, 0xFD};

    const int u32RespLen = 5;
    uint8_t u8RespData[5];
    return SendReceiveBytes(pContext, bytes, sizeof(bytes), u8RespData, u32RespLen);
}

esp_err_t STM32PROTOCOL_CmdErase(const STM32PROTOCOL_SContext* pContext)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    ESP_LOGI(TAG_STM_PRO, "ERASE MEMORY");
    const uint8_t bytes[] = {0x43, 0xBC};

    const int u32RespLen = 1;
    uint8_t u8RespData[1];
    const esp_err_t a = SendReceiveBytes(pContext, bytes, sizeof(bytes), u8RespData, u32RespLen);
    if (a == ESP_OK)
    {
        uint8_t params[] = {0xFF, 0x00};
        
        const int u32Resp2Len = 1;
        uint8_t u8Resp2Data[1];
        return SendReceiveBytes(pContext, params, sizeof(params), u8Resp2Data, u32Resp2Len);
    }
    return 0;
}

esp_err_t STM32PROTOCOL_CmdExtErase(const STM32PROTOCOL_SContext* pContext)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    ESP_LOGI(TAG_STM_PRO, "EXTENDED ERASE MEMORY");
    const uint8_t bytes[] = {0x44, 0xBB};
    
    const uint32_t u32RespLen = 1;
    uint8_t u8RespData[1];
    const esp_err_t a = SendReceiveBytes(pContext, bytes, sizeof(bytes), u8RespData, u32RespLen);

    if (a == ESP_OK)
    {
        uint8_t params[] = {0xFF, 0xFF, 0x00};
        const uint32_t u32Resp2Len = 1;
        uint8_t u8Resp2Data[1];
        return SendReceiveBytes(pContext, params, sizeof(params), u8Resp2Data, u32Resp2Len);
    }
    return 0;
}

esp_err_t STM32PROTOCOL_CmdWrite(const STM32PROTOCOL_SContext* pContext)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    ESP_LOGI(TAG_STM_PRO, "WRITE MEMORY");
    const uint8_t bytes[2] = {0x31, 0xCE};
    const uint32_t u32RespLen = 1;
    uint8_t u8RespData[1];
    return SendReceiveBytes(pContext, bytes, sizeof(bytes), u8RespData, u32RespLen);
}

esp_err_t STM32PROTOCOL_CmdRead(const STM32PROTOCOL_SContext* pContext)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    ESP_LOGI(TAG_STM_PRO, "READ MEMORY");
    const uint8_t bytes[2] = {0x11, 0xEE};
    const uint32_t u32RespLen = 1;
    uint8_t u8RespData[1];
    return SendReceiveBytes(pContext, bytes, sizeof(bytes), u8RespData, u32RespLen);
}

esp_err_t STM32PROTOCOL_LoadAddress(const STM32PROTOCOL_SContext* pContext, const char adrMS, const char adrMI, const char adrLI, const char adrLS)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    ESP_LOGI(TAG_STM_PRO, "LoadAddress");
    uint8_t xor = adrMS ^ adrMI ^ adrLI ^ adrLS;
    uint8_t params[] = {adrMS, adrMI, adrLI, adrLS, xor};

    const uint32_t u32RespLen = 1;
    uint8_t u8RespData[1];

    // ESP_LOG_BUFFER_HEXDUMP("LOAD ADDR", params, sizeof(params), ESP_LOG_DEBUG);
    return SendReceiveBytes(pContext, params, sizeof(params), u8RespData, u32RespLen);
}

esp_err_t STM32PROTOCOL_IncrementLoadAddress(const STM32PROTOCOL_SContext* pContext, char *loadAddr)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    loadAddr[2] += 0x1;

    if (loadAddr[2] == 0)
    {
        loadAddr[1] += 0x1;

        if (loadAddr[1] == 0)
        {
            loadAddr[0] += 0x1;
        }
    }
    return ESP_OK;
}

esp_err_t STM32PROTOCOL_FlashPage(const STM32PROTOCOL_SContext* pContext, const char *address, const uint8_t* data)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    ESP_LOGI(TAG_STM_PRO, "Flashing Page");

    STM32PROTOCOL_CmdWrite(pContext);

    STM32PROTOCOL_LoadAddress(pContext, address[0], address[1], address[2], address[3]);
    //ESP_LOG_BUFFER_HEXDUMP("FLASH PAGE", data, 256, ESP_LOG_DEBUG);
    uint8_t xor = 0xFF;
    uint8_t sz = 0xFF;

    SendData(pContext, &sz, 1);
    for (int i = 0; i < 256; i++)
    {
        SendData(pContext, &data[i], 1);
        xor ^= data[i];
    }

    SendData(pContext, &xor, 1);
    const int length = WaitForSerialData(pContext, 1, pContext->psConfig->u32SyncDataTimeoutMS);
    if (length < 0)
    {
        ESP_LOGE(TAG_STM_PRO, "Serial Timeout");
        return ESP_FAIL;
    }

    uint8_t uart_data[length];
    const int rxBytes = uart_read_bytes(pContext->psConfig->uart_port, uart_data, length, 1000 / portTICK_RATE_MS);
    if (rxBytes == 0)
    {
        ESP_LOGE(TAG_STM_PRO, "No response");
        return ESP_FAIL;
    }

    if (uart_data[0] == STM32PROTOCOL_NACK)
    {
        ESP_LOGI(TAG_STM_PRO, "Flash failure (NACK received)");
        return ESP_FAIL;
    }
    
    if (uart_data[0] != STM32PROTOCOL_ACK)
    {
        ESP_LOGE(TAG_STM_PRO, "Flash Failure (unknown return)");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG_STM_PRO, "Flash Success");
    return ESP_OK;
}

esp_err_t STM32PROTOCOL_ReadPage(const STM32PROTOCOL_SContext* pContext, const char *address, const uint8_t* data)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    ESP_LOGI(TAG_STM_PRO, "Reading page");
    const uint8_t param[] = {0xFF, 0x00};

    STM32PROTOCOL_CmdRead(pContext);

    STM32PROTOCOL_LoadAddress(pContext, address[0], address[1], address[2], address[3]);

    SendData(pContext, param, sizeof(param));
    const int length = WaitForSerialData(pContext, 257, pContext->psConfig->u32SyncDataTimeoutMS);
    if (length < 0)
    {
        ESP_LOGE(TAG_STM_PRO, "%s", "Serial Timeout");
        return ESP_FAIL;
    }

    uint8_t uart_data[length];
    const int rxBytes = uart_read_bytes(pContext->psConfig->uart_port, uart_data, length, 1000 / portTICK_RATE_MS);
    if (rxBytes == 0)
    {
        ESP_LOGE(TAG_STM_PRO, "No response");
        return ESP_FAIL;
    }

    if (uart_data[0] == STM32PROTOCOL_NACK)
    {
        ESP_LOGE(TAG_STM_PRO, "Failure (NACK received)");
        return ESP_FAIL;
    }
    
    if ( uart_data[0] != STM32PROTOCOL_ACK)
    {
        ESP_LOGE(TAG_STM_PRO, "Flash Failure (unknown return)");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG_STM_PRO, "Success");
    if (memcpy((void *)data, uart_data, 257) == 0)
    {
        return ESP_FAIL;
    }
    return ESP_OK;
}

static void InitGPIO(const STM32PROTOCOL_SContext* pContext)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
	ESP_LOGI(TAG_STM_PRO, "Init GPIO reset: %"PRId32", boot0: %"PRId32", boot1: %"PRId32, 
        (int32_t)pContext->psConfig->reset_pin,
        (int32_t)pContext->psConfig->boot0_pin,
        (int32_t)pContext->psConfig->boot1_pin);

	gpio_reset_pin(pContext->psConfig->reset_pin);
    gpio_set_direction(pContext->psConfig->reset_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pContext->psConfig->reset_pin, HIGH);
	gpio_reset_pin(pContext->psConfig->boot0_pin);
    gpio_set_direction(pContext->psConfig->boot0_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pContext->psConfig->boot0_pin, HIGH);

    if (pContext->psConfig->boot1_pin >= 0)
    {
        gpio_reset_pin(pContext->psConfig->boot1_pin);
        gpio_set_direction(pContext->psConfig->boot1_pin, GPIO_MODE_INPUT);   
    }
}

static void InitFlashUART(const STM32PROTOCOL_SContext* pContext)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
	ESP_LOGI(TAG_STM_PRO,"Init flash UART");
	
    const uart_config_t uart_config = {
        .baud_rate = STM32PROTOCOL_SERIAL_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

    uart_driver_install(pContext->psConfig->uart_port, UART_BUF_SIZE * 2, 0, 0, NULL, 0);

    uart_param_config(pContext->psConfig->uart_port, &uart_config);
    uart_set_pin(pContext->psConfig->uart_port, pContext->psConfig->uart_tx_pin, pContext->psConfig->uart_rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    ESP_LOGI(TAG_STM_PRO,"Initialized Flash UART");
}

static esp_err_t SendReceiveBytes(const STM32PROTOCOL_SContext* pContext, const uint8_t* u8SendData, uint32_t u32SendCount, uint8_t* u8RecvData, uint32_t u32ExpectedRecvCount)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    SendData(pContext, u8SendData, u32SendCount);
    const int length = WaitForSerialData(pContext, u32ExpectedRecvCount, pContext->psConfig->u32SyncDataTimeoutMS);
    if (length < 0)
    {
        ESP_LOGE(TAG_STM_PRO, "Serial Timeout");
        return ESP_FAIL;
    }

    const int rxBytes = uart_read_bytes(pContext->psConfig->uart_port, u8RecvData, length, pdMS_TO_TICKS(pContext->psConfig->u32RecvDataTimeoutMS));
    if (rxBytes == 0)
    {
        ESP_LOGE(TAG_STM_PRO, "Serial receive error");
        return ESP_FAIL;
    }

    if (u8RecvData[0] == STM32PROTOCOL_NACK)
    {
        ESP_LOGE(TAG_STM_PRO, "Serial receive error (NACK returned)");
        return ESP_FAIL;
    }
    
    if (u8RecvData[0] != STM32PROTOCOL_ACK)
    {
        ESP_LOGE(TAG_STM_PRO, "Serial receive error (unknown data received)");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG_STM_PRO, "Serial receive success");
    return ESP_OK;
}

static int SendData(const STM32PROTOCOL_SContext* pContext, const uint8_t* u8OutgoingData, const int count)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    const int txBytes = uart_write_bytes(pContext->psConfig->uart_port, u8OutgoingData, count);
    //logD(logName, "Wrote %d bytes", count);
    return txBytes;
}

static int WaitForSerialData(const STM32PROTOCOL_SContext* pContext, int dataCount, int timeout)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
	int32_t s32CurrTimeout = timeout;
	
    while (s32CurrTimeout > 0)
    {
		size_t length = 0;
        uart_get_buffered_data_len(pContext->psConfig->uart_port, (size_t *)&length);
        if (length >= dataCount)
        {
            return length;
        }
		
        vTaskDelay(1);
		s32CurrTimeout -= pdTICKS_TO_MS(1);
    }
    return -1;
}
