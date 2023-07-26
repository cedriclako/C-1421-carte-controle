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
#include "ParticlesManager.h"
#include "ParamFile.h"

/***** Définitions ******/

#define SECONDS(x) (x*1000)
#define MINUTES(x) (SECONDS(60)*x)
#define CELSIUS_TO_FAHRENHEIT(TEMP) (TEMP*9/5+32)
#define FAHRENHEIT_TO_CELSIUS(TEMP) ((TEMP-32)*5/9)
#define P2F(x) (float)(x/10) //Parameter to float


/***** Variables ******/

typedef enum {
  ZEROING_STEPPER,
  WAITING,
  RELOAD_IGNITION,
  TEMPERATURE_RISE,
  COMBUSTION_HIGH,
  COMBUSTION_LOW,
  COAL_LOW,
  COAL_HIGH,
  OVERTEMP,
  SAFETY,
  MANUAL_CONTROL,

  ALGO_NB_OF_STATE
} State;

typedef struct {

	int8_t i8apertureDegree;
	int8_t i8apertureSetPoint;
	float fSecPerStep;

} AirInput;


typedef struct
{
	///////////////STRUCTURES/////////////////////////

	//Air openings (in degrees) and min/max
	AirInput sPrimary;
	AirInput sGrill;
	AirInput sSecondary;

	//Particles values (ch0, ch1, slope, stdev, etc.)
	const MeasureParticles_t *sParticles;

	//Manual mode params
	const PF_UsrParam *sUserParams;
	//////////////////////////////////////////////////

	////////////////VARIABLES/////////////////////////
	//Temperatures
	float fBaffleTemp;
	float fChamberTemp;
	float fPlenumTemp;

	//Temperature slopes
	float fBaffleDeltaT;
	float fChamberDeltaT;
	///////////////////////////////////////////////////

	/////////////////STATE VARIABLES///////////////////

	bool bThermostatOn;
	bool bInterlockOn;

	bool bstateJustChanged;
	uint32_t u32TimeOfStateEntry_ms;

	uint32_t u32TimeOfMotorRequest_ms;
	uint32_t u32TimeBeforeInPlace_ms;

	bool bReloadRequested;
	uint32_t TimeOfReloadRequest;
	///////////////////////////////////////////////////

} Mobj; // Main application object

/***** Fonctions ******/

extern void Algo_Init(void const * argument);

#endif /* INC_ALGO_H_ */
