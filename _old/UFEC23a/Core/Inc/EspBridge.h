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

void ESPMANAGER_Init();

void ESPMANAGER_Task(void const * argument);

extern osSemaphoreId ESP_UART_SemaphoreHandle;


#endif /* SRC_ESPBRIDGE_H_ */
