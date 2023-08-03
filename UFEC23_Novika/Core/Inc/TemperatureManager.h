#ifndef TEMPERATURE_MAN_H
#define	TEMPERATURE_MAN_H

#include "cmsis_os.h"
#include "stm32f1xx_hal.h"
#include "Algo.h"


//public function
void TemperatureManager(Mobj* Stove, uint32_t time_ms);
void Temperature_update_deltaT(Mobj *stove, uint32_t u32DeltaT_ms);
void Temperature_Init();
float get_BoardTemp(void);

#endif	/* TEMPERATURE_MAN_H  */
