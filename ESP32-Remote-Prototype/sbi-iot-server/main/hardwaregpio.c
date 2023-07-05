#include "hardwaregpio.h"

void HARDWAREGPIO_Init()
{
    // Sanity LED
    gpio_set_direction(HWGPIO_SANITYLED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(HWGPIO_SANITYLED_PIN, false);

     /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    const uart_config_t uart_config = {
        .baud_rate = HWGPIO_BRIDGEUART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    
    int intr_alloc_flags = 0;
     
    // Configure a temporary buffer for the incoming data
    ESP_ERROR_CHECK(uart_param_config(HWGPIO_BRIDGEUART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(HWGPIO_BRIDGEUART_PORT_NUM, HWGPIO_BRIDGEUART_TXD_PIN, HWGPIO_BRIDGEUART_RXD_PIN, HWGPIO_BRIDGEUART_RTS_PIN, HWGPIO_BRIDGEUART_CTS_PIN));
    ESP_ERROR_CHECK(uart_driver_install(HWGPIO_BRIDGEUART_PORT_NUM, HWGPIO_BRIDGEUART_BUFFSIZE * 2, 0, 0, NULL, intr_alloc_flags));
}

void HARDWAREGPIO_SetSanity(bool bIsActive)
{
    gpio_set_level(HWGPIO_SANITYLED_PIN, !bIsActive);
}