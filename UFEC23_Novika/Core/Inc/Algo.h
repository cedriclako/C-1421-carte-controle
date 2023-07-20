/*
 * Algo.h
 *
 *  Created on: Jul 12, 2023
 *      Author: crichard
 */

#ifndef INC_ALGO_H_
#define INC_ALGO_H_

#include <stdint.h>
#include <stdbool.h>
#include "AirInput.h"

/***** Définitions ******/

#define SECONDS(x) (x*1000)
#define MINUTES(x) (SECONDS(60)*x)
#define CELSIUS_TO_FAHRENHEIT(TEMP) (TEMP*9/5+32)
#define FAHRENHEIT_TO_CELSIUS(TEMP) ((TEMP-32)*5/9)


/***** Variables ******/

typedef enum {
  ZEROING_STEPPER,
  WAITING,
  RELOAD_IGNITION,
  TEMPERATURE_RISE,
  COMBUSTION_HIGH,
  COMBUSTION_LOW,
  COMBUSTION_SUPERLOW,
  COAL_LOW,
  FLAME_LOSS,
  COAL_HIGH,
  OVERTEMP,
  SAFETY,
  PRODUCTION_TEST,
  MANUAL_CONTROL,

  NB_OF_STATE
} State;



typedef struct
{

	AirInput primary;
	AirInput grill;
	AirInput secondary;

	float fBaffleTemp;
	float fChamberTemp;
	float fPlenumTemp;

	float fBaffleDeltaT;
	float fChamberDeltaT;

	bool bThermostatOn;
	bool bInterlockOn;




} Mobj; // Main application object

/***** Fonctions ******/

extern void Algo_Init(void const * argument);

#endif /* INC_ALGO_H_ */
