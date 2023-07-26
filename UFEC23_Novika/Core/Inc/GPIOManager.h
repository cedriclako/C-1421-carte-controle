/*
 * GPIOManager.h
 *
 *  Created on: 25 juill. 2023
 *      Author: crichard
 */

#ifndef INC_GPIOMANAGER_H_
#define INC_GPIOMANAGER_H_
#include <stdbool.h>
#include "Algo.h"

bool GPIO_IsButtonPressed(void);
void GPIOManager(Mobj *stove, uint32_t u32CurrentTime_ms);

#endif /* INC_GPIOMANAGER_H_ */
