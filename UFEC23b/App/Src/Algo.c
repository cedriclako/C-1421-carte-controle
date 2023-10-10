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
#include "FanManager.h"
//#include "ProdTest.h"
//#include "MotorManager.h"
#include "ParticlesManager.h"
#include "EspBridge.h"
#include "TemperatureManager.h"
#include "DebugManager.h"
#include "Algo.h"
#include "GitCommit.h"

extern MessageBufferHandle_t MotorControlsHandle;
extern QueueHandle_t MotorInPlaceHandle;
static bool motors_ready_for_req = false;
static bool bStepperAdjustmentNeeded = false;
static bool bStateExitConditionMet = false;
static State currentState = ZEROING_STEPPER;
static State lastState = ZEROING_STEPPER;
static State nextState = ZEROING_STEPPER;

static const char* m_StateStrings[ALGO_NB_OF_STATE] =
{
	[(int)ZEROING_STEPPER] = "ZEROING_STEP",
	[(int)WAITING] = "WAITING",
	[(int)RELOAD_IGNITION] = "RELOAD_IGNI",
	[(int)TEMPERATURE_RISE] = "TEMP_RISE",
	[(int)COMBUSTION_HIGH] = "COMB_HIGH",
	[(int)COMBUSTION_LOW] = "COMB_LOW",
	[(int)COAL_LOW] = "COAL_LOW",
	[(int)COAL_HIGH] = "COAL_HIGH",
	[(int)OVERTEMP] = "OVERTEMP",
	[(int)SAFETY] = "SAFETY",
	[(int)MANUAL_CONTROL] = "MANUAL"
};

typedef void (*fnComputeAdjustment)(Mobj *stove, uint32_t u32CurrentTime_ms);
typedef void (*fnSuperStateAction)(Mobj *stove);

// Adjustment functions for each state
static fnComputeAdjustment AlgoComputeAdjustment[ALGO_NB_OF_STATE];
static fnSuperStateAction AlgoStateEntryAction[ALGO_NB_OF_STATE];
static fnSuperStateAction AlgoStateExitAction[ALGO_NB_OF_STATE];
const PF_SuperStateParam_t* sStateParams[ALGO_NB_OF_STATE];
const PF_OverHeat_Thresholds_t *sOverheatParams;

static Mobj UFEC23;

static void Algo_fill_state_functions(void);
static void Algo_reload_action(Mobj* stove, uint32_t u32CurrentTime_ms);
static void Algo_Waiting_action(Mobj* stove, uint32_t u32CurrentTime_ms);
static void Algo_tempRise_action(Mobj* stove, uint32_t u32CurrentTime_ms);
static void Algo_combHigh_action(Mobj* stove, uint32_t u32CurrentTime_ms);
static void Algo_combLow_action(Mobj* stove, uint32_t u32CurrentTime_ms);
static void Algo_coalHigh_action(Mobj* stove, uint32_t u32CurrentTime_ms);
static void Algo_coalLow_action(Mobj* stove, uint32_t u32CurrentTime_ms);
static void Algo_manual_action(Mobj* stove, uint32_t u32CurrentTime_ms);
static void Algo_zeroing_action(Mobj* stove, uint32_t u32CurrentTime_ms);
static void Algo_safety_action(Mobj* stove, uint32_t u32CurrentTime_ms);
static void Algo_overtemp_action(Mobj* stove, uint32_t u32CurrentTime_ms);

static void Algo_waiting_entry(Mobj *stove);
static void Algo_reload_entry(Mobj *stove);
static void Algo_zeroing_entry(Mobj *stove);
static void Algo_tempRise_entry(Mobj *stove);
static void Algo_combLow_entry(Mobj *stove);
static void Algo_combHigh_entry(Mobj *stove);
static void Algo_coalLow_entry(Mobj *stove);
static void Algo_coalHigh_entry(Mobj *stove);

//static void Algo_waiting_exit(Mobj *stove);
static void Algo_reload_exit(Mobj *stove);
//static void Algo_zeroing_exit(Mobj *stove);
static void Algo_tempRise_exit(Mobj *stove);
static void Algo_combLow_exit(Mobj *stove);
static void Algo_combHigh_exit(Mobj *stove);
//static void Algo_coalLow_exit(Mobj *stove);
//static void Algo_coalHigh_exit(Mobj *stove);

void Algo_task(Mobj *stove, uint32_t u32CurrentTime_ms);
bool Algo_adjust_steppers_position(Mobj *stove);
void Algo_update_steppers_inPlace_flag(void);
void Algo_stoveInit(Mobj *stove);

void Algo_Init(void const * argument)
{
	printf("\r\n\r\n");
	printf("--------------------------------\r\n");
	printf("BOOTING APP\r\n");
	printf("Commitid: %s, Branch: %s, Dirty: %d\r\n", GITCOMMIT_COMMITID, GITCOMMIT_BRANCH, GITCOMMIT_ISDIRTY);
	printf("--------------------------------\r\n");

	Algo_fill_state_functions();

	PARAMFILE_Init();
	Algo_stoveInit(&UFEC23);
	Temperature_Init();
	ESPMANAGER_Init();
	Particle_Init();
	Fan_Init();
	// We want to be sure the system is ready before accepting to answer to any commands
	ESPMANAGER_SetReady();

    for(;;)
    {
    	GPIOManager(&UFEC23,osKernelSysTick());
    	TemperatureManager(&UFEC23,osKernelSysTick());
    	DebugManager(&UFEC23,osKernelSysTick());
    	ESPMANAGER_Run();
    	ParticlesManager(osKernelSysTick());
    	Algo_task(&UFEC23, osKernelSysTick());
    	Fan_Process(&UFEC23);

    	//osDelay(1);
    }

}

void Algo_task(Mobj *stove, uint32_t u32CurrentTime_ms)
{
	const PF_UsrParam* UsrParam =  PB_GetUserParam();
	const PF_OverHeat_Thresholds_t* OvrhtParams = PB_GetOverheatParams();

	Algo_update_steppers_inPlace_flag(); // Check if motor task is moving or in place

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
			AlgoStateEntryAction[currentState](stove);
		}

	}
	else if(stove->bReloadRequested && (currentState == WAITING || currentState == COMBUSTION_LOW || currentState == COMBUSTION_HIGH ||
			currentState == COAL_LOW || currentState == COAL_HIGH || currentState == TEMPERATURE_RISE))
	{
		if(!stove->bInterlockOn)
		{
			nextState = RELOAD_IGNITION;
			stove->bReloadRequested = false;
			stove->bButtonBlinkRequired = true;
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
					AlgoComputeAdjustment[currentState](stove, u32CurrentTime_ms);
				}
			}
			stove->u32TimeOfComputation_ms = u32CurrentTime_ms;
			PrintOutput(stove, nextState);
		}else if(currentState == MANUAL_CONTROL) // If in manual control, we don't wait the computation time
		{										// But we still loop in the first 'if' once per computation period (to print output)
			if(AlgoComputeAdjustment[currentState] != NULL)
			{
				AlgoComputeAdjustment[currentState](stove, u32CurrentTime_ms);
			}
		}
		// Check if state timed out or if exit conditions are met
		if((((u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms) > MINUTES(sStateParams[currentState]->i32MinimumTimeInStateMinutes)) && bStateExitConditionMet) ||
				((sStateParams[currentState]->i32MaximumTimeInStateMinutes != 0) && ((u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms) > MINUTES(sStateParams[currentState]->i32MaximumTimeInStateMinutes))))
		{

			if(AlgoStateExitAction[currentState] != NULL)
			{
				AlgoStateExitAction[currentState](stove);
			}else
			{
				nextState = ZEROING_STEPPER;
			}
			bStateExitConditionMet = false;
		}
	}

	if(bStepperAdjustmentNeeded) // If an adjustment is requested, send configs to motors
	{

		if(Algo_adjust_steppers_position(stove))
		{
			stove->sPrimary.u8aperturePosSteps = RANGE(PF_PRIMARY_MINIMUM_OPENING,stove->sPrimary.u8apertureCmdSteps,PF_PRIMARY_FULL_OPEN);
			stove->sGrill.u8aperturePosSteps = RANGE(PF_GRILL_MINIMUM_OPENING,stove->sGrill.u8apertureCmdSteps,PF_GRILL_FULL_OPEN);
			stove->sSecondary.u8aperturePosSteps = RANGE(PF_SECONDARY_MINIMUM_OPENING,stove->sSecondary.u8apertureCmdSteps,PF_SECONDARY_FULL_OPEN);

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

	for(uint8_t i = 0;i < ALGO_NB_OF_STATE;i++)
	{
		sStateParams[i] = PB_GetSuperStateParams(i);
	}

	stove->u32TimeOfStateEntry_ms = 0;
	stove->u32TimeOfAdjustment_ms = 0;
	stove->u32TimeOfComputation_ms = 0;
	stove->u32TimeSinceCombEntry_ms = 0;
	stove->bReloadRequested = false;
	stove->bButtonBlinkRequired = false;
	stove->bstateJustChanged = true;
	stove->bSafetyOn = false;
	stove->TimeOfReloadRequest = 0;

	stove->sPrimary.u8apertureCmdSteps = MOTOR_HOME_CMD;
	stove->sGrill.u8apertureCmdSteps = MOTOR_HOME_CMD;
	stove->sSecondary.u8apertureCmdSteps = MOTOR_HOME_CMD;

	Algo_adjust_steppers_position(stove);
}

///////////////////////// STATE MACHINE  //////////////////////////////////////////////////////////////////////


//** STATE: ZEROING STEPPER **//
static void Algo_zeroing_entry(Mobj *stove)
{
	stove->sPrimary.u8apertureCmdSteps = 0;
	stove->sPrimary.fSecPerStep = 0;
	stove->sGrill.u8apertureCmdSteps = 0;
	stove->sGrill.fSecPerStep = 0;
	stove->sSecondary.u8apertureCmdSteps = 0;
	stove->sSecondary.fSecPerStep = 0;
	bStepperAdjustmentNeeded = true;
}

static void Algo_zeroing_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{

	if(motors_ready_for_req)
	{
		nextState = WAITING;
	}
}

//static void Algo_zeroing_exit(Mobj *stove)
//{

//}
//** END: ZEROING STEPPER **//

//** STATE: WAITING **//
static void Algo_waiting_entry(Mobj *stove)
{
	const PF_WaitingParam_t *sParam = PB_GetWaitingParams();

	if((stove->fBaffleTemp > P2F(sParam->fTempToSkipWaiting)))
	{
		nextState = TEMPERATURE_RISE;
	}

}

static void Algo_Waiting_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
	const PF_WaitingParam_t *sParam = PB_GetWaitingParams();

	if(!stove->bInterlockOn)
	{
		if((stove->fBaffleTemp > P2F(sParam->fTempToQuitWaiting)))
		{
			stove->bReloadRequested = true;
		}
	}

}

//static void Algo_waiting_exit(Mobj *stove)
//{

//}

//** END: ZEROING STEPPER **//

//** STATE: RELOAD / IGNITION **//
static void Algo_reload_entry(Mobj* stove)
{
	const PF_ReloadParam_t *sParam = PB_GetReloadParams();
	stove->u32TimeSinceCombEntry_ms = 0;

	stove->sPrimary.u8apertureCmdSteps = sParam->sPrimary.i32Max;
	stove->sPrimary.fSecPerStep = 0; // force aperture
	stove->sGrill.u8apertureCmdSteps = sParam->sGrill.i32Max;
	stove->sGrill.fSecPerStep = 0; // force aperture
	stove->sSecondary.u8apertureCmdSteps = sParam->sSecondary.i32Max;
	stove->sSecondary.fSecPerStep = 0; // force aperture
	bStepperAdjustmentNeeded = true;

	if((stove->fBaffleTemp > P2F(sParam->fTempToSkipReload)))
	{
		nextState = TEMPERATURE_RISE;
	}

}

static void Algo_reload_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
	const PF_ReloadParam_t *sParam = PB_GetReloadParams();

	if((stove->fBaffleTemp > P2F(sParam->fTempToQuitReload)))
	{
		nextState = TEMPERATURE_RISE;
	}
}

static void Algo_reload_exit(Mobj *stove)
{

}
//** END: RELOAD / IGNITION**//


//** STATE: TEMPERATURE RISE **//
static void Algo_tempRise_entry(Mobj* stove)
{

	const PF_TriseParam_t *sParam = PB_GetTRiseParams();

	stove->u32TimeSinceCombEntry_ms = 0;

	stove->sPrimary.u8apertureCmdSteps = sParam->sPrimary.i32Max;
	stove->sPrimary.fSecPerStep = 0; // force aperture
	stove->sGrill.u8apertureCmdSteps = sParam->sGrill.i32Max;
	stove->sGrill.fSecPerStep = 0; // force aperture
	stove->sSecondary.u8apertureCmdSteps = sParam->sSecondary.i32Max;
	stove->sSecondary.fSecPerStep = 0; // force aperture
	bStepperAdjustmentNeeded = true;
}

static void Algo_tempRise_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
	const PF_TriseParam_t *sParam = PB_GetTRiseParams();
	static uint32_t u32TimeOfMajorCorr = 0;
	const PF_StepperStepsPerSec_t *sSpeedParams =  PB_SpeedParams();

	if((stove->bThermostatOn && stove->fBaffleTemp > P2F(sParam->fTempToCombHigh)) ||	//2023-08-11: if hot enough, go to comb states
			(!stove->bThermostatOn && stove->fBaffleTemp > P2F(sParam->fTempToCombLow)))
	{
		nextState = stove->bThermostatOn ? COMBUSTION_HIGH : COMBUSTION_LOW;
		return;
	}

	// Case(s) we want to wait
	if((u32TimeOfMajorCorr != 0 && (u32CurrentTime_ms - u32TimeOfMajorCorr < MINUTES(1))) || 	// We just made a correction
			(stove->fBaffleDeltaT < (P2F1DEC(sParam->sTempSlope.fTarget - sParam->sTempSlope.fTolerance)))) // Temperature rises abnormally slow
	{
		return;
	}

	if(stove->fBaffleTemp >P2F(sParam->fTempToStartReg))
	{
		if((stove->sParticles->fparticles) > (P2F(sParam->sParticles.fTarget + sParam->sParticles.fAbsMaxDiff)) &&
				(stove->fBaffleDeltaT > (P2F1DEC(sParam->sTempSlope.fTarget + sParam->sTempSlope.fTolerance))))
		{
			if(stove->sGrill.u8apertureCmdSteps > 15)
			{
				stove->sGrill.u8apertureCmdSteps /= 2;
				stove->sGrill.fSecPerStep = 0;
			}
			else if(stove->sGrill.u8apertureCmdSteps > sParam->sGrill.i32Min)
			{
				stove->sGrill.u8apertureCmdSteps = sParam->sGrill.i32Min;
				stove->sGrill.fSecPerStep = 0;
			}
			else
			{
				stove->sPrimary.u8apertureCmdSteps = 75;
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
		if(stove->sGrill.u8apertureCmdSteps > sParam->sGrill.i32Min)
		{
			stove->sGrill.u8apertureCmdSteps--;
			stove->sGrill.fSecPerStep = P2F1DEC(sSpeedParams->fFast);
		}else
		{

			if(stove->sPrimary.u8apertureCmdSteps-- <= 75)
			{
				nextState = stove->bThermostatOn ? COMBUSTION_HIGH : COMBUSTION_LOW;
			}
			stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fFast);
		}

		bStepperAdjustmentNeeded = true;
	}


}

static void Algo_tempRise_exit(Mobj *stove)
{

}
//** END: TEMPERATURE RISE **//


//** STATE: COMBUSTION LOW **//
static void Algo_combLow_entry(Mobj *stove)
{
	const PF_CombustionParam_t *sParam = PB_GetCombLowParams();

	stove->sPrimary.u8apertureCmdSteps = sParam->sPrimary.i32Max;
	stove->sPrimary.fSecPerStep = 0; // force aperture
	stove->sGrill.u8apertureCmdSteps = sParam->sGrill.i32Min;
	stove->sGrill.fSecPerStep = 0; // force aperture
	stove->sSecondary.u8apertureCmdSteps = sParam->sSecondary.i32Max;
	stove->sSecondary.fSecPerStep = 0; // force aperture
	bStepperAdjustmentNeeded = true;
}

static void Algo_combLow_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
	const PF_CombustionParam_t *sParam = PB_GetCombLowParams();

	static uint32_t u32MajorCorrectionTime_ms = 0;
	const PF_StepperStepsPerSec_t *sSpeedParams =  PB_SpeedParams();

	if(stove->fBaffleTemp < P2F(sParam->sTemperature.fTarget - sParam->sTemperature.fTolerance))
	{
		if((stove->fBaffleDeltaT < P2F1DEC(sParam->sTempSlope.fTarget - sParam->sTempSlope.fAbsMaxDiff)) ||
				((stove->sParticles->u16stDev > sParam->sPartStdev.fAbsMaxDiff) && stove->sParticles->fparticles > P2F1DEC(sParam->sParticles.fTarget + sParam->sParticles.fTolerance)))
		{
			if(u32MajorCorrectionTime_ms - u32CurrentTime_ms > SECONDS(30))
			{
				if(stove->sPrimary.u8apertureCmdSteps *= 2 > sParam->sPrimary.i32Max)
				{
					stove->sPrimary.u8apertureCmdSteps = sParam->sPrimary.i32Max;
				}
				stove->sPrimary.fSecPerStep = 0; // force aperture
				bStepperAdjustmentNeeded = true;

				u32MajorCorrectionTime_ms = u32CurrentTime_ms;
				return;
			}
		}

		if(stove->fBaffleDeltaT < P2F1DEC(sParam->sTempSlope.fTarget - sParam->sTempSlope.fTolerance))
		{
			if(motors_ready_for_req || stove->sPrimary.fSecPerStep == P2F1DEC(sSpeedParams->fVerySlow))
			{
				if(stove->sPrimary.u8apertureCmdSteps++ > sParam->sPrimary.i32Max)//Open by one step
				{
					stove->sPrimary.u8apertureCmdSteps = sParam->sPrimary.i32Max;
				}

				if(stove->sParticles->u16stDev > sParam->sPartStdev.fTolerance)
				{
					stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fFast);
				}else
				{
					stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fNormal);
				}

				bStepperAdjustmentNeeded = true;
			}

		}
	}else if(stove->fBaffleTemp < P2F(sParam->sTemperature.fTarget + sParam->sTemperature.fAbsMaxDiff))
	{
		if(fabs(stove->fBaffleDeltaT) < P2F1DEC(sParam->sTempSlope.fTarget + sParam->sTempSlope.fTolerance))
		{
			if(motors_ready_for_req)
			{
				if(stove->sPrimary.u8apertureCmdSteps-- < sParam->sPrimary.i32Min)//Close by one step
				{
					stove->sPrimary.u8apertureCmdSteps = sParam->sPrimary.i32Min;
				}

				if(stove->sParticles->u16stDev > sParam->sPartStdev.fTolerance)
				{
					stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fNormal);
				}else if(stove->sParticles->fparticles > P2F(sParam->sParticles.fTarget + sParam->sParticles.fTolerance))
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
		if(stove->fBaffleDeltaT > P2F1DEC(sParam->sTempSlope.fTarget + sParam->sTempSlope.fAbsMaxDiff))
		{
			if(motors_ready_for_req || (stove->sPrimary.fSecPerStep == P2F1DEC(sSpeedParams->fSlow)) || (stove->sPrimary.fSecPerStep == P2F1DEC(sSpeedParams->fVerySlow)))
			{
				if(stove->sPrimary.u8apertureCmdSteps-- < sParam->sPrimary.i32Min)
				{
					stove->sPrimary.u8apertureCmdSteps = sParam->sPrimary.i32Min;
				}

				if(stove->sParticles->u16stDev > sParam->sPartStdev.fTolerance)
				{
					stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fFast);
				}else
				{
					stove->sPrimary.fSecPerStep = P2F1DEC(sSpeedParams->fNormal);
				}

				bStepperAdjustmentNeeded = true;
			}
		}

		if(stove->fBaffleDeltaT > P2F1DEC(sParam->sTempSlope.fTarget - sParam->sTempSlope.fTolerance))
		{
			if(motors_ready_for_req || stove->sPrimary.fSecPerStep == P2F1DEC(sSpeedParams->fVerySlow))
			{
				if(stove->sPrimary.u8apertureCmdSteps-- < sParam->sPrimary.i32Min)
				{
					stove->sPrimary.u8apertureCmdSteps = sParam->sPrimary.i32Min;
				}

				if(stove->sParticles->u16stDev > sParam->sPartStdev.fTolerance)
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

	if(stove->fBaffleTemp < P2F(sParam->sTemperature.fTarget - 2*sParam->sTemperature.fAbsMaxDiff) &&
			stove->sParticles->fparticles < P2F1DEC(sParam->sParticles.fTarget + sParam->sParticles.fTolerance) &&
			stove->sParticles->u16stDev < (uint32_t)sParam->sPartStdev.fTolerance)
	{

		bStateExitConditionMet = true;
	}




}

static void Algo_combLow_exit(Mobj *stove)
{
	if(bStateExitConditionMet)
	{
		nextState = COAL_LOW;
	}
	else
	{
		nextState = ZEROING_STEPPER;
	}
}

//** END: COMBUSTION LOW **//

//** STATE: COMBUSTION HIGH **//
static void Algo_combHigh_entry(Mobj *stove)
{
	//const PF_CombustionParam_t *sParam = PB_GetCombHighParams();


}


static void Algo_combHigh_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
	//const PF_CombustionParam_t *sParam = PB_GetCombHighParams();
}

static void Algo_combHigh_exit(Mobj *stove)
{

}

//** END: COMBUSTION HIGH **//

//** STATE: COAL LOW **//
static void Algo_coalLow_entry(Mobj *stove)
{
	//const PF_CoalParam_t *sParam = PB_GetCoalLowParams();

}

static void Algo_coalLow_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
	const PF_CoalParam_t *sParam = PB_GetCoalLowParams();

	if(stove->sParticles->u16stDev > sParam->sPartStdev.fTolerance)
	{
		nextState = COMBUSTION_LOW;
		return;
	}


	if((stove->fBaffleTemp < P2F( sParam->sTemperature.fTarget)) && stove->sGrill.u8apertureCmdSteps != sParam->sGrill.i32Max)
	{
		stove->sGrill.u8apertureCmdSteps = sParam->sGrill.i32Max;
		stove->sGrill.fSecPerStep = 0; // force aperture
		bStepperAdjustmentNeeded = true;

	}


		//Ne pas être en coal s'il y a de la fumée (MUST NOT)
	// Timer de combustion (low et high)
		//TODO: Regarder après avoir ouvert la grille, temp et particules++ --> ****(en tout temps pour fumée)si remontée, on retourne en comb
	//Ça pourrait être le entry action, mettre des thresholds temp, parts

	if(u32CurrentTime_ms - stove->u32TimeOfAdjustment_ms > MINUTES(sParam->i32TimeBeforeMovingPrim) && (stove->sPrimary.u8apertureCmdSteps != sParam->sPrimary.i32Min))
	{
		stove->sPrimary.u8apertureCmdSteps = sParam->sPrimary.i32Min;
		stove->sPrimary.fSecPerStep = 0; // force aperture
		bStepperAdjustmentNeeded = true;

	}

	if(u32CurrentTime_ms - stove->u32TimeOfAdjustment_ms > MINUTES(sParam->i32TimeBeforeMovingSec) && (stove->sSecondary.u8apertureCmdSteps != sParam->sSecondary.i32Min))
	{
		stove->sSecondary.u8apertureCmdSteps = sParam->sSecondary.i32Min;
		stove->sSecondary.fSecPerStep = 0; // force aperture
		bStepperAdjustmentNeeded = true;

	}


}

//static void Algo_coalLow_exit(Mobj *stove)
//{

//}
//** END: COAL LOW **//

//** STATE: COAL HIGH **//
static void Algo_coalHigh_entry(Mobj *stove)
{
	//const PF_CoalParam_t *sParams = PB_GetCoalHighParams();
}

static void Algo_coalHigh_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
	//const PF_CoalParam_t *sParams = PB_GetCoalHighParams();
}

//static void Algo_coalHigh_exit(Mobj *stove)
//{

//}
//** END: COAL HIGH **//

static void Algo_manual_action(Mobj* stove, uint32_t u32CurrentTime_ms)
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

	if(stove->sPrimary.u8apertureCmdSteps != sManParam->s32ManualPrimary ||
			stove->sGrill.u8apertureCmdSteps != sManParam->s32ManualGrill ||
			stove->sSecondary.u8apertureCmdSteps != sManParam->s32ManualSecondary)
	{
		stove->sPrimary.u8apertureCmdSteps = sManParam->s32ManualPrimary;
		stove->sPrimary.fSecPerStep = 0; // force aperture
		stove->sGrill.u8apertureCmdSteps = sManParam->s32ManualGrill;
		stove->sGrill.fSecPerStep = 0; // force aperture
		stove->sSecondary.u8apertureCmdSteps = sManParam->s32ManualSecondary;
		stove->sSecondary.fSecPerStep = 0; // force aperture
		bStepperAdjustmentNeeded = true;
	}


}

static void Algo_safety_action(Mobj* stove, uint32_t u32CurrentTime_ms)
{
	if(!stove->bSafetyOn)
	{
		nextState = lastState;
	}
}

static void Algo_overtemp_action(Mobj* stove, uint32_t u32CurrentTime_ms)
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

	AlgoStateExitAction[ZEROING_STEPPER] = NULL;
	AlgoStateExitAction[WAITING] = NULL;
	AlgoStateExitAction[RELOAD_IGNITION] = Algo_reload_exit;
	AlgoStateExitAction[TEMPERATURE_RISE] = Algo_tempRise_exit;
	AlgoStateExitAction[COMBUSTION_HIGH] = Algo_combHigh_exit;
	AlgoStateExitAction[COMBUSTION_LOW] = Algo_combLow_exit;
	AlgoStateExitAction[COAL_LOW] = NULL;
	AlgoStateExitAction[COAL_HIGH] = NULL;
	AlgoStateExitAction[OVERTEMP] = NULL;
	AlgoStateExitAction[SAFETY] = NULL;
	AlgoStateExitAction[MANUAL_CONTROL] = NULL;

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
		stove->sPrimary.u8apertureCmdSteps,
		(uint8_t)(stove->sPrimary.fSecPerStep*10),
		stove->sGrill.u8apertureCmdSteps,
		(uint8_t)(stove->sGrill.fSecPerStep*10),
		stove->sSecondary.u8apertureCmdSteps,
		(uint8_t)(stove->sSecondary.fSecPerStep*10)

	};

	if(!xMessageBufferSend(MotorControlsHandle,cmd,NUMBER_OF_STEPPER_CMDS,0))
	{
		return false;
	}
	return true;
}

const Mobj* ALGO_GetObjData()
{
	return &UFEC23;
}

State ALGO_GetCurrentState()
{
	return currentState;
}

const char* ALGO_GetStateString(State state)
{
	if (state >= ALGO_NB_OF_STATE)
		return "-- N/A --";
	return m_StateStrings[currentState];
}
