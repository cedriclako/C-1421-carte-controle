#include "hardwaregpio.h"
#include "uartbridge.h"

static uint8_t m_u8UARTDriverBuffer[HWGPIO_BRIDGEUART_BUFFSIZE];

void UARTBRIDGE_Init()
{
    

}

void UARTBRIDGE_Handler()
{

    // Read data from the UART
    int len = uart_read_bytes(HWGPIO_BRIDGEUART_PORT_NUM, m_u8UARTDriverBuffer, HWGPIO_BRIDGEUART_BUFFSIZE, 0);
    
    // Write data back to the UART
    uart_write_bytes(HWGPIO_BRIDGEUART_PORT_NUM, (const char *) m_u8UARTDriverBuffer, len);
}