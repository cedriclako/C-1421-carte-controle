#ifndef TEMPERATURE_MAN_H
#define	TEMPERATURE_MAN_H

#include "cmsis_os.h"
#include "stm32f1xx_hal.h"


//public function
void ReadTemperatureTask(void const * argument);
void TemperatureManager(void const * argument);
int get_BoardTemp(void);

#endif	/* TEMPERATURE_MAN_H  */
