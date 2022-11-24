#include "DebugPort.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal.h"
#include <sys/unistd.h>
#include <sys/errno.h>

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;

//void stdio_setup(int no_init)
//{
//    // Turn off buffers, so I/O occurs immediately
//    setvbuf(stdin, NULL, _IONBF, 0);
//    setvbuf(stdout, NULL, _IONBF, 0);
//    setvbuf(stderr, NULL, _IONBF, 0);
//}
#ifdef __cplusplus
 extern "C" {
#endif

int __io_putchar(int ch) {
  HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}






#ifdef __cplusplus
}
#endif
