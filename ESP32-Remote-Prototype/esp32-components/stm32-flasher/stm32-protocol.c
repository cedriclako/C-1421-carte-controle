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
#define TAG "STMPROTOCOL"
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
static void DeinitFlashUART(const STM32PROTOCOL_SContext* pContext);

static int WaitForSerialData(const STM32PROTOCOL_SContext* pContext, int dataCount, int timeout);
static int SendData(const STM32PROTOCOL_SContext* pContext, const uint8_t* u8OutgoingData, const int count);
static esp_err_t SendReceiveBytes(const STM32PROTOCOL_SContext* pContext, const uint8_t* u8SendData, uint32_t u32SendCount, uint8_t* u8RecvData, uint32_t u32ExpectedRecvCount);

void STM32PROTOCOL_Init(STM32PROTOCOL_SContext* pContext, const STM32PROTOCOL_SConfig* pConfig)
{
	ESP_LOGI(TAG, "Init STM32 flasher");
	pContext->psConfig = pConfig;
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
}

esp_err_t STM32PROTOCOL_SetupSTM(const STM32PROTOCOL_SContext* pContext)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    // Remove garbage from the buffer
    ESP_LOGI(TAG, "STM32PROTOCOL_ResetSTM");
    STM32PROTOCOL_RETURNNOTOK(STM32PROTOCOL_ResetSTM(pContext, true));
    
    // Attempt several time to sync ....
    ESP_LOGI(TAG, "STM32PROTOCOL_CmdSync");
    STM32PROTOCOL_RETURNNOTOK(STM32PROTOCOL_CmdSync(pContext));

    ESP_LOGI(TAG, "STM32PROTOCOL_CmdGet");
    STM32PROTOCOL_RETURNNOTOK(STM32PROTOCOL_CmdGet(pContext));
    ESP_LOGI(TAG, "STM32PROTOCOL_CmdVersion");
    STM32PROTOCOL_RETURNNOTOK(STM32PROTOCOL_CmdVersion(pContext));
    ESP_LOGI(TAG, "STM32PROTOCOL_CmdId");
    STM32PROTOCOL_RETURNNOTOK(STM32PROTOCOL_CmdId(pContext));
    ESP_LOGI(TAG, "STM32PROTOCOL_CmdErase");
    STM32PROTOCOL_RETURNNOTOK(STM32PROTOCOL_CmdErase(pContext));
    ESP_LOGI(TAG, "STM32PROTOCOL_CmdExtErase");
    STM32PROTOCOL_RETURNNOTOK(STM32PROTOCOL_CmdExtErase(pContext));
	return ESP_OK;
}

esp_err_t STM32PROTOCOL_StartConn(const STM32PROTOCOL_SContext* pContext)
{
    ESP_LOGI(TAG, "STM32PROTOCOL_StartConn");

    if (pContext->psConfig->bInitGPIO) {
		InitGPIO(pContext);
    }
	
	if (pContext->psConfig->bInitUART) {
		InitFlashUART(pContext);
    }
    return ESP_OK;
}

esp_err_t STM32PROTOCOL_EndConn(const STM32PROTOCOL_SContext* pContext)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);

    ESP_LOGI(TAG, "STM32PROTOCOL_ResetToApp");
    STM32PROTOCOL_RETURNNOTOK(STM32PROTOCOL_ResetToApp(pContext));

    uart_flush_input(pContext->psConfig->uart_port);
    ESP_LOGI(TAG, "Ending Connection");

	if (pContext->psConfig->bInitUART) {
		DeinitFlashUART(pContext);
    }

	return ESP_OK;
}

esp_err_t STM32PROTOCOL_ResetSTM(const STM32PROTOCOL_SContext* pContext, bool bIsBootloaderMode)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
	ESP_LOGI(TAG, "Starting RESET Procedure");
	gpio_set_level(pContext->psConfig->reset_pin, LOW);

    // Test: BOOT0 = HIGH during process, then LOW at the end
    //       BOOT1 = LOW during process, HIGH at the END
    //       RESET = LOW on RESET, HIGH after
    // According to the datasheet, putting BOOTPIN0 to HIGH and BOOTPIN1 TO LOW will boot the system in bootloader mode.
    if (pContext->psConfig->boot0_pin >= 0) {
        gpio_set_level(pContext->psConfig->boot0_pin, (bIsBootloaderMode ? HIGH : LOW));
    }
    if (pContext->psConfig->boot1_pin >= 0) {
	    gpio_set_level(pContext->psConfig->boot1_pin, (bIsBootloaderMode ? LOW : HIGH));
    }
	vTaskDelay(pdMS_TO_TICKS(1500));
	gpio_set_level(pContext->psConfig->reset_pin, HIGH);
	vTaskDelay(pdMS_TO_TICKS(3000));
	ESP_LOGI(TAG, "Finished RESET Procedure");
    return ESP_OK;
}

esp_err_t STM32PROTOCOL_ResetToApp(const STM32PROTOCOL_SContext* pContext)
{
    if (pContext->psConfig->boot1_pin >= 0) {
        gpio_set_level(pContext->psConfig->boot1_pin, HIGH);
    }
    if (pContext->psConfig->boot0_pin >= 0) {
        gpio_set_level(pContext->psConfig->boot0_pin, LOW);
    }

    if (pContext->psConfig->reset_pin >= 0) {
	    gpio_set_level(pContext->psConfig->reset_pin, LOW);
    }
	vTaskDelay(pdMS_TO_TICKS(250));
    if (pContext->psConfig->reset_pin >= 0) {
	    gpio_set_level(pContext->psConfig->reset_pin, HIGH);
    }
        
    // Zero-initialize the config structure.
    gpio_config_t io_conf = {0};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = 0;
    if (pContext->psConfig->reset_pin >= 0) {
        io_conf.pin_bit_mask |= (1ULL<<pContext->psConfig->reset_pin);
    }
    if (pContext->psConfig->boot0_pin >= 0) {
        io_conf.pin_bit_mask |= (1ULL<<pContext->psConfig->boot0_pin);
    }
    if (pContext->psConfig->boot1_pin >= 0) {
        io_conf.pin_bit_mask |= (1ULL<<pContext->psConfig->boot1_pin);
    }
    return ESP_OK;
}

esp_err_t STM32PROTOCOL_CmdSync(const STM32PROTOCOL_SContext* pContext)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    ESP_LOGI(TAG, "SYNC");
    const uint8_t u8SendData[] = { 0x7F };
    const uint32_t u32RespLen = 1;
    uint8_t u8RespData[1];
    return SendReceiveBytes(pContext, u8SendData, sizeof(u8SendData), u8RespData, u32RespLen);
}

esp_err_t STM32PROTOCOL_CmdGet(const STM32PROTOCOL_SContext* pContext)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    ESP_LOGI(TAG, "GET");
    const uint8_t bytes[] = {0x00, 0xFF};

    const int u32RespLen = 15;
    uint8_t u8RespData[15];
    return SendReceiveBytes(pContext, bytes, sizeof(bytes), u8RespData, u32RespLen);
}

esp_err_t STM32PROTOCOL_CmdVersion(const STM32PROTOCOL_SContext* pContext)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    ESP_LOGI(TAG, "GET VERSION & READ PROTECTION STATUS");
    const uint8_t  bytes[] = {0x01, 0xFE};

    const int u32RespLen = 5;
    uint8_t u8RespData[5];
    return SendReceiveBytes(pContext, bytes, sizeof(bytes), u8RespData, u32RespLen);
}

esp_err_t STM32PROTOCOL_CmdId(const STM32PROTOCOL_SContext* pContext)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    ESP_LOGI(TAG, "CHECK ID");
    const uint8_t bytes[] = {0x02, 0xFD};

    const int u32RespLen = 5;
    uint8_t u8RespData[5];
    return SendReceiveBytes(pContext, bytes, sizeof(bytes), u8RespData, u32RespLen);
}

esp_err_t STM32PROTOCOL_CmdErase(const STM32PROTOCOL_SContext* pContext)
{
    /*  It seems to be one based, so Byte1 is the first one.
    Byte 1: 0x43
    Byte 2: 0xBC
    Wait for ACK
    Byte 3: 0xFF or number of pages to be erased – 1 (0 ≤N ≤ maximum number of pages)
    Byte 4: 0x00 (in case of global erase) or ((N + 1 bytes (page numbers) and then checksum
    XOR (N, N+1 bytes) */
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    ESP_LOGI(TAG, "ERASE MEMORY");
    const uint8_t bytes[] = {0x43, 0xBC};

    const int u32RespLen = 1;
    uint8_t u8RespData[1];
    const esp_err_t a = SendReceiveBytes(pContext, bytes, sizeof(bytes), u8RespData, u32RespLen);
    if (a == ESP_OK)
    {
        const uint8_t params[] = {0xFF, 0x00};
        const int u32Resp2Len = 1;
        uint8_t u8Resp2Data[1];
        return SendReceiveBytes(pContext, params, sizeof(params), u8Resp2Data, u32Resp2Len);
    }
    return 0;
}

esp_err_t STM32PROTOCOL_CmdExtErase(const STM32PROTOCOL_SContext* pContext)
{
    /*  It seems to be one based, so Byte1 is the first one.
    Byte 1: 0x44
    Byte 2: 0xBB
    Wait for ACK
    Bytes 3-4: Special erase (0xFFFF, 0xFFFE, or 0xFFFD)
    or
    Number of pages to be erased (N+1 where 0 ≤ N < Maximum number of
    pages).
    Remaining
    bytes:
    Checksum of bytes 3-4 in case of special erase (0x00 if 0xFFFF or 0x01 if
    0xFFFE, or 0x02 if 0xFFFD)
    or
    (2 x (N + 1)) bytes (page numbers coded on two bytes MSB first) and then
    the checksum for bytes 3-4 and all the following bytes */
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    ESP_LOGI(TAG, "EXTENDED ERASE MEMORY");
    const uint8_t bytes[] = {0x44, 0xBB};
    
    const uint32_t u32RespLen = 1;
    uint8_t u8RespData[1];
    const esp_err_t a = SendReceiveBytes(pContext, bytes, sizeof(bytes), u8RespData, u32RespLen);
    if (a == ESP_OK)
    {
        const uint8_t params[] = {0xFF, 0xFF, 0x00};
        const uint32_t u32Resp2Len = 1;
        uint8_t u8Resp2Data[1];
        return SendReceiveBytes(pContext, params, sizeof(params), u8Resp2Data, u32Resp2Len);
    }
    return 0;
}

esp_err_t STM32PROTOCOL_CmdWrite(const STM32PROTOCOL_SContext* pContext)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    ESP_LOGI(TAG, "WRITE MEMORY");
    const uint8_t bytes[2] = {0x31, 0xCE};
    const uint32_t u32RespLen = 1;
    uint8_t u8RespData[1];
    return SendReceiveBytes(pContext, bytes, sizeof(bytes), u8RespData, u32RespLen);
}

esp_err_t STM32PROTOCOL_CmdRead(const STM32PROTOCOL_SContext* pContext)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    ESP_LOGI(TAG, "READ MEMORY");
    const uint8_t bytes[2] = {0x11, 0xEE};
    const uint32_t u32RespLen = 1;
    uint8_t u8RespData[1];
    return SendReceiveBytes(pContext, bytes, sizeof(bytes), u8RespData, u32RespLen);
}

esp_err_t STM32PROTOCOL_LoadAddress(const STM32PROTOCOL_SContext* pContext, const char adrMS, const char adrMI, const char adrLI, const char adrLS)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    ESP_LOGI(TAG, "LoadAddress");
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
    ESP_LOGI(TAG, "Flashing Page");
    /*  It seems to be one based, so Byte1 is the first one.
        Byte 1: 0x31
        Byte 2: 0xCE
        Wait for ACK
        Bytes 3 to 6: Start address (byte 3: MSB, byte 6: LSB)
        Byte 7: Checksum: XOR (byte3, byte4, byte5, byte6)
        Wait for ACK
        Byte 8: Number of bytes to be received (0 < N ≤ 255)
        N +1 data bytes: Max 256 bytes
        Checksum byte: XOR (N, N+1 data bytes)
    */
    STM32PROTOCOL_CmdWrite(pContext);

    STM32PROTOCOL_LoadAddress(pContext, address[0], address[1], address[2], address[3]);
    //ESP_LOG_BUFFER_HEXDUMP("FLASH PAGE", data, 256, ESP_LOG_DEBUG);
    uint8_t xor = 0xFF;
    uint8_t sz = 0xFF;
    // the bootloader interpret it as 255+1 (256 bytes) to be written.
    // according to the documentation.
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
        ESP_LOGE(TAG, "Serial Timeout");
        return ESP_FAIL;
    }

    uint8_t uart_data[length];
    const int rxBytes = uart_read_bytes(pContext->psConfig->uart_port, uart_data, length, 1000 / portTICK_PERIOD_MS);
    if (rxBytes == 0)
    {
        ESP_LOGE(TAG, "No response");
        return ESP_FAIL;
    }

    if (uart_data[0] == STM32PROTOCOL_NACK)
    {
        ESP_LOGI(TAG, "Flash failure (NACK received)");
        return ESP_FAIL;
    }
    
    if (uart_data[0] != STM32PROTOCOL_ACK)
    {
        ESP_LOGE(TAG, "Flash Failure (unknown return)");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t STM32PROTOCOL_ReadPage(const STM32PROTOCOL_SContext* pContext, const char *address, const uint8_t* data)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    ESP_LOGI(TAG, "Reading page");
    const uint8_t param[] = {0xFF, 0x00};

    STM32PROTOCOL_CmdRead(pContext);

    STM32PROTOCOL_LoadAddress(pContext, address[0], address[1], address[2], address[3]);

    SendData(pContext, param, sizeof(param));
    const int length = WaitForSerialData(pContext, 257, pContext->psConfig->u32SyncDataTimeoutMS);
    if (length < 0)
    {
        ESP_LOGE(TAG, "%s", "Serial Timeout");
        return ESP_FAIL;
    }

    uint8_t uart_data[length];
    const int rxBytes = uart_read_bytes(pContext->psConfig->uart_port, uart_data, length, 1000 / portTICK_PERIOD_MS);
    if (rxBytes == 0)
    {
        ESP_LOGE(TAG, "No response");
        return ESP_FAIL;
    }

    if (uart_data[0] == STM32PROTOCOL_NACK)
    {
        ESP_LOGE(TAG, "Failure (NACK received)");
        return ESP_FAIL;
    }
    
    if ( uart_data[0] != STM32PROTOCOL_ACK)
    {
        ESP_LOGE(TAG, "Flash Failure (unknown return)");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Success");
    if (memcpy((void *)data, uart_data, 257) == 0)
    {
        return ESP_FAIL;
    }
    return ESP_OK;
}

static void InitGPIO(const STM32PROTOCOL_SContext* pContext)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
	ESP_LOGI(TAG, "Init GPIO reset: %"PRId32", boot0: %"PRId32", boot1: %"PRId32, 
        (int32_t)pContext->psConfig->reset_pin,
        (int32_t)pContext->psConfig->boot0_pin,
        (int32_t)pContext->psConfig->boot1_pin);

    // Zero-initialize the config structure.
    gpio_config_t io_conf = {0};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = 0;
    if (pContext->psConfig->reset_pin >= 0) {
        io_conf.pin_bit_mask |= (1ULL<<(uint64_t)pContext->psConfig->reset_pin);
    }
    if (pContext->psConfig->boot0_pin >= 0) {
        io_conf.pin_bit_mask |= (1ULL<<(uint64_t)pContext->psConfig->boot0_pin);
    }
    if (pContext->psConfig->boot1_pin >= 0) {
        io_conf.pin_bit_mask |= (1ULL<<(uint64_t)pContext->psConfig->boot1_pin);
    }
    //configure GPIO with the given settings
    gpio_config(&io_conf);
}

static void InitFlashUART(const STM32PROTOCOL_SContext* pContext)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
	ESP_LOGI(TAG,"Init flash UART");
	
    const uart_config_t uart_config = {
        .baud_rate = STM32PROTOCOL_SERIAL_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

    uart_driver_install(pContext->psConfig->uart_port, UART_BUF_SIZE * 2, 0, 0, NULL, 0);

    uart_param_config(pContext->psConfig->uart_port, &uart_config);
    uart_set_pin(pContext->psConfig->uart_port, pContext->psConfig->uart_tx_pin, pContext->psConfig->uart_rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    ESP_LOGI(TAG,"Initialized Flash UART");
}

static void DeinitFlashUART(const STM32PROTOCOL_SContext* pContext)
{
    uart_driver_delete(pContext->psConfig->uart_port);
}

static esp_err_t SendReceiveBytes(const STM32PROTOCOL_SContext* pContext, const uint8_t* u8SendData, uint32_t u32SendCount, uint8_t* u8RecvData, uint32_t u32ExpectedRecvCount)
{
    STM32PROTOCOL_ASSERTCONTEXT(pContext);
    // Clear the input buffer before attempting to write ...
    uart_flush_input(pContext->psConfig->uart_port);
    SendData(pContext, u8SendData, u32SendCount);
    const int length = WaitForSerialData(pContext, u32ExpectedRecvCount, pContext->psConfig->u32SyncDataTimeoutMS);
    if (length <= 0)
    {
        ESP_LOGE(TAG, "Serial Timeout");
        return ESP_FAIL;
    }

    if (length > u32ExpectedRecvCount)
    {
        ESP_LOGE(TAG, "The input buffer is too small");
        return ESP_FAIL;
    }

    const int rxBytes = uart_read_bytes(pContext->psConfig->uart_port, u8RecvData, length, pdMS_TO_TICKS(pContext->psConfig->u32RecvDataTimeoutMS));
    if (rxBytes == 0)
    {
        ESP_LOGE(TAG, "Serial receive error");
        return ESP_FAIL;
    }

    if (u8RecvData[0] == STM32PROTOCOL_NACK)
    {
        ESP_LOGE(TAG, "Serial receive error (NACK returned)");
        return ESP_FAIL;
    }
    
    if (u8RecvData[0] != STM32PROTOCOL_ACK)
    {
        ESP_LOGE(TAG, "Serial receive error (unknown data received) 0x%"PRIX8, u8RecvData[0]);
        return ESP_FAIL;
    }
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
