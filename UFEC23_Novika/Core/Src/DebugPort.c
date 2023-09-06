#include "main.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal.h"
#include <sys/unistd.h>
#include <sys/errno.h>
#include "DebugPort.h"

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

extern UART_HandleTypeDef huart2;

int __io_putchar(int ch) {
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}




#ifdef __cplusplus
}
#endif
