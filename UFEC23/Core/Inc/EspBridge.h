/*
 * EspBridge.h
 *
 *  Created on: 21 nov. 2022
 *      Author: crichard
 */

#ifndef SRC_ESPBRIDGE_H_
#define SRC_ESPBRIDGE_H_

#include <stdlib.h>
#include <stdio.h>

extern osSemaphoreId ESP_UART_SemaphoreHandle;

void EspManager(void const * argument);


#endif /* SRC_ESPBRIDGE_H_ */
