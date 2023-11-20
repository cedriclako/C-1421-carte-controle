#include "OTAUploadSTM32.h"
#include "hardwaregpio.h"
#include "stm32-process.h"
#include "stm32-protocol.h"
#include "../fwconfig.h"
#include "../uartbridge/stovemb.h"
#include "../uartbridge/uartbridge.h"

#define TAG "OTAUploadSTM32"

esp_err_t OTAUPLOADSTM32_postotauploadSTM32_handler(httpd_req_t *req)
{
    esp_err_t err = ESP_FAIL;
    CHECK_FOR_ACCESS_OR_RETURN();

    // Stop the UART bridge
    UARTBRIDGE_Stop();

    const char* szError = NULL;

    FILE* flash_file = NULL;

    httpd_resp_set_type(req, "application/json");
    const char* filename = FWCONFIG_SPIFF_ROOTPATH"/stm32.bin";

    ESP_LOGI(TAG, "file_postotauploadSTM32_handler / uri: %s, filename: %s", req->uri, filename);

    // Prepare the STM32
    flash_file = fopen(filename, "w");
    if (flash_file == NULL)
    {
        szError = "Unable to open file for writing operation";
        goto ERROR;
    }

    int binary_file_length = 0;
    int n = 0;
    
    do
    {
        n = httpd_req_recv(req, (char*)g_u8Buffers, HTTPSERVER_BUFFERSIZE);
        if (n == 0) {
            break;
        }
        else if (n < 0) {
            /* Respond with 500 Internal Server Error */
            szError = "Failed to receive";
            goto ERROR;
        }
        fwrite (g_u8Buffers , sizeof(uint8_t), n, flash_file);
        ESP_LOGI(TAG, "file_postotauploadSTM32_handler / receiving: %d bytes", n);

        binary_file_length += n;
    }
    while (n > 0);

    if (binary_file_length == 0) {
        szError = "The file is empty";
        goto ERROR;
    }

    // Close the handler, so the other process can reopen the file
    if (flash_file != NULL) {
        fclose(flash_file);
        flash_file = NULL;
    }

    // Initialize upload
    STM32PROTOCOL_SContext sContext;
    STM32PROTOCOL_SConfig sConfig = STM32PROTOCOL_SCONFIG_INIT;
    sConfig.bInitGPIO = true;
    sConfig.bInitUART = true;

    sConfig.reset_pin = HWGPIO_STM32_RESET_PIN;
    sConfig.boot0_pin = HWGPIO_STM32_BOOT0_PIN;
    sConfig.boot1_pin = HWGPIO_STM32_BOOT1_PIN;

    sConfig.uart_port = HWGPIO_BRIDGEUART_PORT_NUM;
    sConfig.uart_tx_pin = HWGPIO_BRIDGEUART_TXD_PIN;
    sConfig.uart_rx_pin = HWGPIO_BRIDGEUART_RXD_PIN;

    STM32PROTOCOL_Init(&sContext, &sConfig);

    if (ESP_OK != STM32PROCESS_FlashSTM(&sContext, filename))
    {
        szError = "Unable to flash";
        goto ERROR;
    }

    httpd_resp_send(req, NULL, 0);
    err = ESP_OK;
    goto END;
    ERROR:
    err = ESP_FAIL;
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, ((szError != NULL) ? szError : "Unknown error"));
    END:
    if (flash_file != NULL) {
        fclose(flash_file);
    }

    httpd_resp_set_hdr(req, "Connection", "close");
    // Restore the UART bridge.
    UARTBRIDGE_Start();
    return err;
}
