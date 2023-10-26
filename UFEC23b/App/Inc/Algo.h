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
#define P2F(x) ((float)x) //Parameter to float
#define P2F1DEC(x) ((float)x/10) //Parameter to float with 1 decimal precision
#define P2F2DEC(x) ((float)x/100) //Parameter to float with 2 decimal precision


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
  BOOST,

  ALGO_NB_OF_STATE
} State;

typedef struct {

	uint8_t u8apertureCmdSteps;
	uint8_t u8aperturePosSteps;
	float fSecPerStep;

} AirInput;

typedef enum
{
	Prim_pos = 0,
	prim_spd,
	gril_pos,
	gril_spd,
	sec_pos,
	sec_spd,

	NUMBER_OF_STEPPER_CMDS
}eMotor_commands;

typedef struct
{
	///////////////STRUCTURES/////////////////////////

	//Air openings (in degrees) and min/max
	AirInput sPrimary;
	AirInput sGrill;
	AirInput sSecondary;

	//Particles values (ch0, ch1, slope, stdev, etc.)
	const MeasureParticles_t *sParticles;

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
	bool bDoorOpen;
	bool bSafetyOn;

	bool bstateJustChanged;
	uint32_t u32TimeOfStateEntry_ms;
	uint32_t u32TimeSinceCombEntry_ms;

	uint32_t u32TimeOfAdjustment_ms;
	uint32_t u32TimeOfComputation_ms;

	bool bReloadRequested;
	bool bButtonBlinkRequired;
	uint32_t TimeOfReloadRequest;
	///////////////////////////////////////////////////

} Mobj; // Main application object

/***** Fonctions ******/
#define MOTOR_HOME_CMD 0xEE
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define RANGE(min,val,max) MAX(min,MIN(val,max))
#define UNUSED_PARAM(param)  (void)(param)

extern void Algo_Init(void const * argument);
const Mobj* ALGO_GetObjData();
State ALGO_GetCurrentState();
const char* ALGO_GetStateString(State state);
#endif /* INC_ALGO_H_ */
