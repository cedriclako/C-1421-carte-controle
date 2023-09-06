/*
 * Algo.c
 *
 *  Created on: Jul 12, 2023
 *      Author: crichard
 */

#include "ParamFile.h"
#include "cmsis_os.h"
#include "main.h"
#include "DebugPort.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "message_buffer.h"
#include "GPIOManager.h"
//#include "ProdTest.h"
//#include "MotorManager.h"
#include "ParticlesManager.h"
#include "EspBridge.h"
#include "TemperatureManager.h"
#include "DebugManager.h"
#include "Algo.h"


extern MessageBufferHandle_t MotorControlsHandle;
extern QueueHandle_t MotorInPlaceHandle;
static bool motors_ready_for_req = false;
static bool bStepperAdjustmentNeeded = false;
static State currentState = ZEROING_STEPPER;
static State lastState = ZEROING_STEPPER;
static State nextState = ZEROING_STEPPER;


typedef void (*fnComputeAdjustment)(Mobj *stove,const PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms);
typedef void (*fnStateEntryAction)(Mobj *stove,const  PF_StateParam_t* sParams);

// Adjustment functions for each state
static fnComputeAdjustment AlgoComputeAdjustment[ALGO_NB_OF_STATE];
static fnStateEntryAction AlgoStateEntryAction[ALGO_NB_OF_STATE];
const PF_StateParam_t* sStateParams[ALGO_NB_OF_STATE];
static PF_StateParam_t sStatedummy;
const PF_OverHeat_Thresholds_t *sOverheatParams;

static void Algo_fill_state_functions(void);
static void Algo_reload_action(Mobj* stove, const PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms);
static void Algo_Waiting_action(Mobj* stove, const PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms);
static void Algo_tempRise_action(Mobj* stove, const PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms);
static void Algo_combHigh_action(Mobj* stove, const PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms);
static void Algo_combLow_action(Mobj* stove, const PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms);
static void Algo_coalHigh_action(Mobj* stove, const PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms);
static void Algo_coalLow_action(Mobj* stove, const PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms);
static void Algo_manual_action(Mobj* stove, const PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms);
static void Algo_zeroing_action(Mobj* stove, const PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms);
static void Algo_safety_action(Mobj* stove, const PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms);
static void Algo_overtemp_action(Mobj* stove, const PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms);

static void Algo_waiting_entry(Mobj *stove,const  PF_StateParam_t* sParams);
static void Algo_reload_entry(Mobj *stove,const  PF_StateParam_t* sParams);
static void Algo_zeroing_entry(Mobj *stove,const  PF_StateParam_t* sParams);
static void Algo_tempRise_entry(Mobj *stove,const  PF_StateParam_t* sParams);
static void Algo_combLow_entry(Mobj *stove,const  PF_StateParam_t* sParams);
static void Algo_combHigh_entry(Mobj *stove,const  PF_StateParam_t* sParams);
static void Algo_coalLow_entry(Mobj *stove,const  PF_StateParam_t* sParams);
static void Algo_coalHigh_entry(Mobj *stove,const  PF_StateParam_t* sParams);

void Algo_task(Mobj *stove, uint32_t u32CurrentTime_ms);
bool Algo_adjust_steppers_position(Mobj *stove);
void Algo_update_steppers_inPlace_flag(void);
void Algo_stoveInit(Mobj *stove);

void Algo_Init(void const * argument)
{
	static Mobj UFEC23;
	Algo_fill_state_functions();

	PARAMFILE_Init();
	Algo_stoveInit(&UFEC23);
	Temperature_Init();
	ESPMANAGER_Init();
	Particle_Init();


	// Print all parameters into the debug file
	for(uint32_t ix = 0; ix < PARAMFILE_GetParamEntryCount(); ix++)
	{
	  const PFL_SParameterItem* pParamItem = PARAMFILE_GetParamEntryByIndex(ix);
	  if (pParamItem == NULL)
		  continue;

	  char tmp[128+1];
	  int32_t s32Value;
	  PFL_GetValueInt32(&PARAMFILE_g_sHandle, pParamItem->szKey, &s32Value);
	  snprintf(tmp, sizeof(tmp), "%s | %d (default: %d, min: %d, max: %d)", pParamItem->szKey, (int)s32Value, (int)pParamItem->uType.sInt32.s32Default, (int)pParamItem->uType.sInt32.s32Min, (int)pParamItem->uType.sInt32.s32Max);
	  printf(tmp);
	}


    for(;;)
    {
    	GPIOManager(&UFEC23,osKernelSysTick());
    	TemperatureManager(&UFEC23,osKernelSysTick());
    	DebugManager(&UFEC23,osKernelSysTick());
    	ESPMANAGER_Task();
    	ParticlesManager(osKernelSysTick());
    	Algo_task(&UFEC23, osKernelSysTick());
    	osDelay(1);
    }

}

void Algo_task(Mobj *stove, uint32_t u32CurrentTime_ms)
{
	const PF_UsrParam* UsrParam =  PB_GetUserParam();
	const PF_OverHeat_Thresholds_t* OvrhtParams = PB_GetOverheatParams();

	Algo_update_steppers_inPlace_flag();

	if((currentState != MANUAL_CONTROL) && UsrParam->s32ManualOverride == 1)
	{
		nextState = MANUAL_CONTROL;
	}
	else if((currentState != SAFETY) && stove->bSafetyOn)
	{
		nextState = SAFETY;
	}
	else if((currentState != OVERTEMP) && (currentState != SAFETY) && (
			(stove->fBaffleTemp > P2F(OvrhtParams->OverheatBaffle))  ||
			(stove->fChamberTemp > P2F(OvrhtParams->OverheatChamber)) ||
			(stove->fPlenumTemp > P2F(OvrhtParams->OverheatPlenum))) )
	{
		nextState = OVERTEMP;
	}
	else if(stove->bstateJustChanged) // If first loop in state, perform entry action
	{
		stove->u32TimeOfStateEntry_ms = u32CurrentTime_ms;
		stove->bstateJustChanged = false;

		if(AlgoStateEntryAction[currentState] != NULL)
		{
			AlgoStateEntryAction[currentState](stove, sStateParams[currentState]);
		}

	}
	else if(stove->bReloadRequested && (currentState == WAITING || currentState == COMBUSTION_LOW || currentState == COMBUSTION_HIGH ||
			currentState == COAL_LOW || currentState == COAL_HIGH))
	{
		if(!stove->bInterlockOn)
		{
			nextState = RELOAD_IGNITION;
			stove->bReloadRequested = false;
			stove->TimeOfReloadRequest = u32CurrentTime_ms;
		}
	}
	else // When we get here, check if it's time to compute an adjustment
	{
		if((u32CurrentTime_ms - stove->u32TimeOfComputation_ms) > UsrParam->s32TimeBetweenComputations_ms)
		{
			Temperature_update_deltaT(stove,(u32CurrentTime_ms - stove->u32TimeOfComputation_ms));
			if((u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms) > SECONDS(sStateParams[currentState]->i32EntryWaitTimeSeconds))
			{
				if(AlgoComputeAdjustment[currentState] != NULL)
				{
					AlgoComputeAdjustment[currentState](stove, sStateParams[currentState], u32CurrentTime_ms);
				}
			}
			stove->u32TimeOfComputation_ms = u32CurrentTime_ms;
			PrintOutput(stove, nextState);
		}else if(currentState == MANUAL_CONTROL) // If in manual control, we don't wait the computation time
		{										// But we still loop in the first 'if' once per computation period (to print output)
			if(AlgoComputeAdjustment[currentState] != NULL)
			{
				AlgoComputeAdjustment[currentState](stove, sStateParams[currentState], u32CurrentTime_ms);
			}
		}
	}

	if(bStepperAdjustmentNeeded) // If an adjustment is requested, send configs to motors
	{

		if(Algo_adjust_steppers_position(stove))
		{
			stove->sPrimary.i8aperturePosSteps = RANGE(PF_PRIMARY_MINIMUM_OPENING,stove->sPrimary.i8apertureCmdSteps,PF_PRIMARY_FULL_OPEN);
			stove->sGrill.i8aperturePosSteps = RANGE(PF_GRILL_MINIMUM_OPENING,stove->sGrill.i8apertureCmdSteps,PF_GRILL_FULL_OPEN);
			stove->sSecondary.i8aperturePosSteps = RANGE(PF_SECONDARY_MINIMUM_OPENING,stove->sSecondary.i8apertureCmdSteps,PF_SECONDARY_FULL_OPEN);

			bStepperAdjustmentNeeded = false;
			stove->u32TimeOfAdjustment_ms = u32CurrentTime_ms;
		}
	}

	if(nextState != currentState) // Perform state change if requested
	{
		lastState = currentState;
		currentState = nextState;
		stove->bstateJustChanged = true;

		if((nextState == COMBUSTION_HIGH || nextState == COMBUSTION_LOW) && stove->u32TimeSinceCombEntry_ms == 0)
		{
			stove->u32TimeSinceCombEntry_ms = u32CurrentTime_ms;
		}
	}

}

void Algo_stoveInit(Mobj *stove)
{
	stove->sParticles = ParticlesGetObject(); // Get pointer to particles Structure
	sOverheatParams = PB_GetOverheatParams();
	sStateParams[WAITING] = PB_GetWaitingParams();
	sStateParams[RELOAD_IGNITION] = PB_GetReloadParams();
	sStateParams[TEMPERATURE_RISE] = PB_GetTRiseParams();
	sStateParams[COMBUSTION_LOW] = PB_GetCombLowParams();
	sStateParams[COMBUSTION_HIGH] = PB_GetCombHighParams();
	sStateParams[COAL_LOW] = PB_GetCoalLowParams();
	sStateParams[COAL_HIGH] = PB_GetCoalHighParams();

	sStatedummy.i32EntryWaitTimeSeconds = 0;

	sStateParams[ZEROING_STEPPER] = &sStatedummy;
	sStateParams[OVERTEMP] = &sStatedummy;
	sStateParams[SAFETY] = &sStatedummy;
	sStateParams[MANUAL_CONTROL] = &sStatedummy;

	stove->u32TimeOfStateEntry_ms = 0;
	stove->u32TimeOfAdjustment_ms = 0;
	stove->u32TimeOfComputation_ms = 0;
	stove->u32TimeSinceCombEntry_ms = 0;
	stove->bReloadRequested = false;
	stove->bstateJustChanged = true;
	stove->bSafetyOn = false;
	stove->TimeOfReloadRequest = 0;
}

///////////////////////// STATE MACHINE  //////////////////////////////////////////////////////////////////////


//** STATE: ZEROING STEPPER **//
static void Algo_zeroing_entry(Mobj *stove,const  PF_StateParam_t* sParams)
{
	stove->sPrimary.i8apertureCmdSteps = 0;
	stove->sPrimary.fSecPerStep = 0;
	stove->sGrill.i8apertureCmdSteps = 0;
	stove->sGrill.fSecPerStep = 0;
	stove->sSecondary.i8apertureCmdSteps = 0;
	stove->sSecondary.fSecPerStep = 0;
	bStepperAdjustmentNeeded = true;
}

static void Algo_zeroing_action(Mobj* stove,const  PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms)
{

	if(motors_ready_for_req)
	{
		nextState = WAITING;
	}
}
//** END: ZEROING STEPPER **//

//** STATE: WAITING **//
static void Algo_waiting_entry(Mobj *stove,const  PF_StateParam_t* sParams)
{
	if((stove->fBaffleTemp > P2F(sParams->sTemperature.fAbsMaxDiff)))
	{
		nextState = TEMPERATURE_RISE;
	}

}

static void Algo_Waiting_action(Mobj* stove,const  PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms)
{
	if(!stove->bInterlockOn)
	{
		if((stove->fBaffleTemp > P2F(sParams->sTemperature.fTarget)))
		{
			stove->bReloadRequested = true;
		}
	}

}
//** END: ZEROING STEPPER **//

//** STATE: RELOAD / IGNITION **//
static void Algo_reload_entry(Mobj* stove,const  PF_StateParam_t* sParams)
{
	stove->u32TimeSinceCombEntry_ms = 0;

	stove->sPrimary.i8apertureCmdSteps = sParams->sPrimary.i32Max;
	stove->sPrimary.fSecPerStep = 0; // force aperture
	stove->sGrill.i8apertureCmdSteps = sParams->sGrill.i32Max;
	stove->sGrill.fSecPerStep = 0; // force aperture
	stove->sSecondary.i8apertureCmdSteps = sParams->sSecondary.i32Max;
	stove->sSecondary.fSecPerStep = 0; // force aperture
	bStepperAdjustmentNeeded = true;

	if((stove->fBaffleTemp > P2F(sParams->sTemperature.fAbsMaxDiff)))
	{
		nextState = TEMPERATURE_RISE;
	}

}

static void Algo_reload_action(Mobj* stove,const  PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms)
{

	if((stove->fBaffleTemp > P2F(sParams->sTemperature.fTarget)))
	{
		nextState = TEMPERATURE_RISE;
	}
}
//** END: RELOAD / IGNITION**//


//** STATE: TEMPERATURE RISE **//
static void Algo_tempRise_entry(Mobj* stove,const  PF_StateParam_t* sParams)
{
	stove->u32TimeSinceCombEntry_ms = 0;

	stove->sPrimary.i8apertureCmdSteps = sParams->sPrimary.i32Max;
	stove->sPrimary.fSecPerStep = 0; // force aperture
	stove->sGrill.i8apertureCmdSteps = sParams->sGrill.i32Max;
	stove->sGrill.fSecPerStep = 0; // force aperture
	stove->sSecondary.i8apertureCmdSteps = sParams->sSecondary.i32Max;
	stove->sSecondary.fSecPerStep = 0; // force aperture
	bStepperAdjustmentNeeded = true;
}

static void Algo_tempRise_action(Mobj* stove,const  PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms)
{
	static uint32_t u32TimeOfMajorCorr = 0;
	const PF_StepperStepsPerSec_t *sSpeedParams =  PB_SpeedParams();

	if((stove->bThermostatOn && stove->fBaffleTemp > sParams->sTemperature.fAbsMaxDiff) ||	//2023-08-11: if hot enough, go to comb states
			(!stove->bThermostatOn && stove->fBaffleTemp > sParams->sTemperature.fTarget))
	{
		nextState = stove->bThermostatOn ? COMBUSTION_HIGH : COMBUSTION_LOW;
		return;
	}

	// Case(s) we want to wait
	if((u32TimeOfMajorCorr != 0 && (u32CurrentTime_ms - u32TimeOfMajorCorr < MINUTES(1))) || 	// We just made a correction
			(stove->fBaffleDeltaT < (P2F1DEC(sParams->sTempSlope.fTarget - sParams->sTempSlope.fTolerance)))) // Temperature rises abnormally slow
	{
		return;
	}

	if(stove->fBaffleTemp >P2F(sParams->i32FreeParam1)) // Here, i32FreeParam1 -> Temp to start regulating with particles
	{
		if((stove->sParticles->fparticles) > (P2F(sParams->sParticles.fTarget + sParams->sParticles.fAbsMaxDiff)) &&
				(stove->fBaffleDeltaT > (P2F1DEC(sParams->sTempSlope.fTarget + sParams->sTempSlope.fTolerance))))
		{
			if(stove->sGrill.i8apertureCmdSteps > 15)
			{
				stove->sGrill.i8apertureCmdSteps /= 2;
				stove->sGrill.fSecPerStep = 0;
			}
			else if(stove->sGrill.i8apertureCmdSteps > sParams->sGrill.i32Min)
			{
				stove->sGrill.i8apertureCmdSteps = sParams->sGrill.i32Min;
				stove->sGrill.fSecPerStep = 0;
			}
			else
			{
				stove->sPrimary.i8apertureCmdSteps = 75;
				stove->sPrimary.fSecPerStep = 0;
				nextState = stove->bThermostatOn ? COMBUSTION_HIGH : COMBUSTION_LOW;
			}



			bStepperAdjustmentNeeded = true;
			u32TimeOfMajorCorr = u32CurrentTime_ms;
			return;
		}
	}

	if(motors_ready_for_req)
	{
		if(stove->sGrill.i8apertureCmdSteps > sParams->sGrill.i32Min)
		{
			stove->sGrill.i8apertureCmdSteps--;
			stove->sGrill.fSecPerStep = P2F1DEC(sSpeedParams->fFast);
		}else
		{

			if(stove->sPrimary.i8apertureCmdSteps-- <= 75)
			{
				nextState = stove->bThermostatOn ? COMBUSTION_HIGH : COMBUSTION_LOW;
			}
			stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fFast);
		}

		bStepperAdjustmentNeeded = true;
	}


}
//** END: TEMPERATURE RISE **//


//** STATE: COMBUSTION LOW **//
static void Algo_combLow_entry(Mobj *stove,const  PF_StateParam_t* sParams)
{
	stove->sPrimary.i8apertureCmdSteps = sParams->sPrimary.i32Max;
	stove->sPrimary.fSecPerStep = 0; // force aperture
	stove->sGrill.i8apertureCmdSteps = sParams->sGrill.i32Min;
	stove->sGrill.fSecPerStep = 0; // force aperture
	stove->sSecondary.i8apertureCmdSteps = sParams->sSecondary.i32Max;
	stove->sSecondary.fSecPerStep = 0; // force aperture
	bStepperAdjustmentNeeded = true;
}

static void Algo_combLow_action(Mobj* stove, const  PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms)
{
	static uint32_t u32MajorCorrectionTime_ms = 0;
	const PF_StepperStepsPerSec_t *sSpeedParams =  PB_SpeedParams();

	if(stove->fBaffleTemp < P2F(sParams->sTemperature.fTarget - sParams->sTemperature.fTolerance))
	{
		if((stove->fBaffleDeltaT < P2F1DEC(sParams->sTempSlope.fTarget - sParams->sTempSlope.fAbsMaxDiff)) ||
				((stove->sParticles->u16stDev > sParams->sPartStdev.fAbsMaxDiff) && stove->sParticles->fparticles > P2F1DEC(sParams->sParticles.fTarget + sParams->sParticles.fTolerance)))
		{
			if(u32MajorCorrectionTime_ms - u32CurrentTime_ms > SECONDS(30))
			{
				if(stove->sPrimary.i8apertureCmdSteps *= 2 > sParams->sPrimary.i32Max)
				{
					stove->sPrimary.i8apertureCmdSteps = sParams->sPrimary.i32Max;
				}
				stove->sPrimary.fSecPerStep = 0; // force aperture
				bStepperAdjustmentNeeded = true;

				u32MajorCorrectionTime_ms = u32CurrentTime_ms;
				return;
			}
		}

		if(stove->fBaffleDeltaT < P2F1DEC(sParams->sTempSlope.fTarget - sParams->sTempSlope.fTolerance))
		{
			if(motors_ready_for_req || stove->sPrimary.fSecPerStep == P2F1DEC(sSpeedParams->fVerySlow))
			{
				if(stove->sPrimary.i8apertureCmdSteps++ > sParams->sPrimary.i32Max)//Open by one step
				{
					stove->sPrimary.i8apertureCmdSteps = sParams->sPrimary.i32Max;
				}

				if(stove->sParticles->u16stDev > sParams->sPartStdev.fTolerance)
				{
					stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fFast);
				}else
				{
					stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fNormal);
				}

				bStepperAdjustmentNeeded = true;
			}

		}
	}else if(stove->fBaffleTemp < P2F(sParams->sTemperature.fTarget + sParams->sTemperature.fAbsMaxDiff))
	{
		if(fabs(stove->fBaffleDeltaT) < P2F1DEC(sParams->sTempSlope.fTarget + sParams->sTempSlope.fTolerance))
		{
			if(motors_ready_for_req)
			{
				if(stove->sPrimary.i8apertureCmdSteps-- < sParams->sPrimary.i32Min)//Close by one step
				{
					stove->sPrimary.i8apertureCmdSteps = sParams->sPrimary.i32Min;
				}

				if(stove->sParticles->u16stDev > sParams->sPartStdev.fTolerance)
				{
					stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fNormal);
				}else if(stove->sParticles->fparticles > P2F(sParams->sParticles.fTarget + sParams->sParticles.fTolerance))
				{
					stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fSlow);
				}else
				{
					stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fVerySlow);
				}


				bStepperAdjustmentNeeded = true;
			}

		}
	}else
	{
		if(stove->fBaffleDeltaT > P2F1DEC(sParams->sTempSlope.fTarget + sParams->sTempSlope.fAbsMaxDiff))
		{
			if(motors_ready_for_req || (stove->sPrimary.fSecPerStep == P2F1DEC(sSpeedParams->fSlow)) || (stove->sPrimary.fSecPerStep == P2F1DEC(sSpeedParams->fVerySlow)))
			{
				if(stove->sPrimary.i8apertureCmdSteps-- < sParams->sPrimary.i32Min)
				{
					stove->sPrimary.i8apertureCmdSteps = sParams->sPrimary.i32Min;
				}

				if(stove->sParticles->u16stDev > sParams->sPartStdev.fTolerance)
				{
					stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fFast);
				}else
				{
					stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fNormal);
				}

				bStepperAdjustmentNeeded = true;
			}
		}

		if(stove->fBaffleDeltaT > P2F1DEC(sParams->sTempSlope.fTarget - sParams->sTempSlope.fTolerance))
		{
			if(motors_ready_for_req || stove->sPrimary.fSecPerStep == P2F1DEC(sSpeedParams->fVerySlow))
			{
				if(stove->sPrimary.i8apertureCmdSteps-- < sParams->sPrimary.i32Min)
				{
					stove->sPrimary.i8apertureCmdSteps = sParams->sPrimary.i32Min;
				}

				if(stove->sParticles->u16stDev > sParams->sPartStdev.fTolerance)
				{
					stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fFast);
				}else
				{
					stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fNormal);
				}

				bStepperAdjustmentNeeded = true;
			}
		}


	}

	if(u32CurrentTime_ms - stove->u32TimeSinceCombEntry_ms > MINUTES(sParams->i32MaximumTimeInStateMinutes) &&
			stove->fBaffleTemp < P2F(sParams->sTemperature.fTarget - 2*sParams->sTemperature.fAbsMaxDiff) &&
			stove->sParticles->fparticles < P2F1DEC(sParams->sParticles.fTarget + sParams->sParticles.fTolerance) &&
			stove->sParticles->u16stDev < (uint32_t)sParams->sPartStdev.fTolerance)
	{

		nextState = COAL_LOW;
	}




}
//** END: COMBUSTION LOW **//

//** STATE: COMBUSTION HIGH **//
static void Algo_combHigh_entry(Mobj *stove,const  PF_StateParam_t* sParams)
{

}


static void Algo_combHigh_action(Mobj* stove,const  PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms)
{

}
//** END: COMBUSTION HIGH **//

//** STATE: COAL LOW **//
static void Algo_coalLow_entry(Mobj *stove,const  PF_StateParam_t* sParams)
{

}

static void Algo_coalLow_action(Mobj* stove,const  PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms)
{
	if(stove->sParticles->u16stDev > sParams->sPartStdev.fTolerance)
	{
		nextState = COMBUSTION_LOW;
		return;
	}


	if((stove->fBaffleTemp < P2F( sParams->sTemperature.fTarget)) && stove->sGrill.i8apertureCmdSteps != sParams->sGrill.i32Max)
	{
		stove->sGrill.i8apertureCmdSteps = sParams->sGrill.i32Max;
		stove->sGrill.fSecPerStep = 0; // force aperture
		bStepperAdjustmentNeeded = true;

	}


		//Ne pas être en coal s'il y a de la fumée (MUST NOT)
	// Timer de combustion (low et high)
		//TODO: Regarder après avoir ouvert la grille, temp et particules++ --> ****(en tout temps pour fumée)si remontée, on retourne en comb
	//Ça pourrait être le entry action, mettre des thresholds temp, parts
													// FreeParam1 used as container (see ParamFile.c)
	if(u32CurrentTime_ms - stove->u32TimeOfAdjustment_ms > MINUTES(sParams->i32FreeParam1) && (stove->sPrimary.i8apertureCmdSteps != sParams->sPrimary.i32Min))
	{
		stove->sPrimary.i8apertureCmdSteps = sParams->sPrimary.i32Min;
		stove->sPrimary.fSecPerStep = 0; // force aperture
		bStepperAdjustmentNeeded = true;

	}
													// FreeParam2 used as container (see ParamFile.c)
	if(u32CurrentTime_ms - stove->u32TimeOfAdjustment_ms > MINUTES(sParams->i32FreeParam2) && (stove->sSecondary.i8apertureCmdSteps != sParams->sSecondary.i32Min))
	{
		stove->sSecondary.i8apertureCmdSteps = sParams->sSecondary.i32Min;
		stove->sSecondary.fSecPerStep = 0; // force aperture
		bStepperAdjustmentNeeded = true;

	}


}
//** END: COAL LOW **//

//** STATE: COAL HIGH **//
static void Algo_coalHigh_entry(Mobj *stove,const  PF_StateParam_t* sParams)
{

}

static void Algo_coalHigh_action(Mobj* stove,const  PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms)
{

}
//** END: COAL HIGH **//

static void Algo_manual_action(Mobj* stove,const  PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms)
{
	const PF_UsrParam* sManParam = PB_GetUserParam();

	if(!(sManParam->s32ManualOverride == 1)) // TODO: Put this in task function
	{
		nextState = lastState;
		if(nextState == ZEROING_STEPPER)
		{
			motors_ready_for_req = false;
		}

		return;
	}

	if(stove->sPrimary.i8apertureCmdSteps != sManParam->s32ManualPrimary ||
			stove->sGrill.i8apertureCmdSteps != sManParam->s32ManualGrill ||
			stove->sSecondary.i8apertureCmdSteps != sManParam->s32ManualSecondary)
	{
		stove->sPrimary.i8apertureCmdSteps = sManParam->s32ManualPrimary;
		stove->sPrimary.fSecPerStep = 0; // force aperture
		stove->sGrill.i8apertureCmdSteps = sManParam->s32ManualGrill;
		stove->sGrill.fSecPerStep = 0; // force aperture
		stove->sSecondary.i8apertureCmdSteps = sManParam->s32ManualSecondary;
		stove->sSecondary.fSecPerStep = 0; // force aperture
		bStepperAdjustmentNeeded = true;
	}


}

static void Algo_safety_action(Mobj* stove, const PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms)
{
	if(!stove->bSafetyOn)
	{
		nextState = lastState;
	}
}

static void Algo_overtemp_action(Mobj* stove, const PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms)
{
	const PF_OverHeat_Thresholds_t* OvrhtParams = PB_GetOverheatParams();

	if((stove->fBaffleTemp < P2F(OvrhtParams->OverheatBaffle))  &&
				(stove->fChamberTemp < P2F(OvrhtParams->OverheatChamber)) &&
				(stove->fPlenumTemp < P2F(OvrhtParams->OverheatPlenumExit)) )
		{
			nextState = lastState;
		}
}

///////////////// Handle and low level functions /////////////////////////////////////////

void Algo_fill_state_functions(void)
{
	AlgoComputeAdjustment[ZEROING_STEPPER] = Algo_zeroing_action;
	AlgoComputeAdjustment[WAITING] = Algo_Waiting_action;
	AlgoComputeAdjustment[RELOAD_IGNITION] = Algo_reload_action;
	AlgoComputeAdjustment[TEMPERATURE_RISE] = Algo_tempRise_action;
	AlgoComputeAdjustment[COMBUSTION_HIGH] = Algo_combHigh_action;
	AlgoComputeAdjustment[COMBUSTION_LOW] = Algo_combLow_action;
	AlgoComputeAdjustment[COAL_LOW] = Algo_coalLow_action;
	AlgoComputeAdjustment[COAL_HIGH] = Algo_coalHigh_action;
	AlgoComputeAdjustment[OVERTEMP] = Algo_overtemp_action;
	AlgoComputeAdjustment[SAFETY] = Algo_safety_action;
	AlgoComputeAdjustment[MANUAL_CONTROL] = Algo_manual_action;

	AlgoStateEntryAction[ZEROING_STEPPER] = Algo_zeroing_entry;
	AlgoStateEntryAction[WAITING] = Algo_waiting_entry;
	AlgoStateEntryAction[RELOAD_IGNITION] = Algo_reload_entry;
	AlgoStateEntryAction[TEMPERATURE_RISE] = Algo_tempRise_entry;
	AlgoStateEntryAction[COMBUSTION_HIGH] = Algo_combHigh_entry;
	AlgoStateEntryAction[COMBUSTION_LOW] = Algo_combLow_entry;
	AlgoStateEntryAction[COAL_LOW] = Algo_coalLow_entry;
	AlgoStateEntryAction[COAL_HIGH] = Algo_coalHigh_entry;
	AlgoStateEntryAction[OVERTEMP] = Algo_zeroing_entry;
	AlgoStateEntryAction[SAFETY] = Algo_zeroing_entry;
	AlgoStateEntryAction[MANUAL_CONTROL] = NULL;

}

void Algo_update_steppers_inPlace_flag(void)
{
	if(!motors_ready_for_req)
	{
		xQueueReceive(MotorInPlaceHandle,&motors_ready_for_req,5);
	}
}

bool Algo_adjust_steppers_position(Mobj *stove)
{
	uint8_t cmd[NUMBER_OF_STEPPER_CMDS] =
	{
		stove->sPrimary.i8apertureCmdSteps,
		(uint8_t)(stove->sPrimary.fSecPerStep*10),
		stove->sGrill.i8apertureCmdSteps,
		(uint8_t)(stove->sGrill.fSecPerStep*10),
		stove->sSecondary.i8apertureCmdSteps,
		(uint8_t)(stove->sSecondary.fSecPerStep*10)

	};

	if(!xMessageBufferSend(MotorControlsHandle,cmd,NUMBER_OF_STEPPER_CMDS,0))
	{
		return false;
	}
	return true;
}
