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
	Algo_update_steppers_inPlace_flag();

	if(currentState != MANUAL_CONTROL && UsrParam->s32ManualOverride == 1)
	{
		nextState = MANUAL_CONTROL;
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
			bStepperAdjustmentNeeded = false;
			stove->u32TimeOfAdjustment_ms = u32CurrentTime_ms;
		}
	}

	if(nextState != currentState) // Perform state change if requested
	{
		lastState = currentState;
		currentState = nextState;
		stove->bstateJustChanged = true;
	}

}


bool Algo_adjust_steppers_position(Mobj *stove)
{
	uint8_t cmd[NUMBER_OF_STEPPER_CMDS] =
	{
		stove->sPrimary.i8apertureSteps,
		(uint8_t)(stove->sPrimary.fSecPerStep*10),
		stove->sGrill.i8apertureSteps,
		(uint8_t)(stove->sGrill.fSecPerStep*10),
		stove->sSecondary.i8apertureSteps,
		(uint8_t)(stove->sSecondary.fSecPerStep*10)

	};
	if(!xMessageBufferSend(MotorControlsHandle,cmd,NUMBER_OF_STEPPER_CMDS,0))
	{
		return false;
	}
	return true;
}

void Algo_update_steppers_inPlace_flag(void)
{
	if(!motors_ready_for_req)
	{
		xQueueReceive(MotorInPlaceHandle,&motors_ready_for_req,5);
	}
}

void Algo_stoveInit(Mobj *stove)
{
	stove->sParticles = ParticlesGetObject();
	stove->sUserParams = PB_GetUserParam();
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
	stove->bReloadRequested = false;
	stove->bstateJustChanged = true;
	stove->TimeOfReloadRequest = 0;
}

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
	AlgoComputeAdjustment[OVERTEMP] = NULL;
	AlgoComputeAdjustment[SAFETY] = NULL;
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


///////////////////////// STATE MACHINE  //////////////////////////////////////////////////////////////////////


//** STATE: ZEROING STEPPER **//
static void Algo_zeroing_entry(Mobj *stove,const  PF_StateParam_t* sParams)
{
	stove->sPrimary.i8apertureSteps = 0;
	stove->sPrimary.fSecPerStep = 0;
	stove->sGrill.i8apertureSteps = 0;
	stove->sGrill.fSecPerStep = 0;
	stove->sSecondary.i8apertureSteps = 0;
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
		if((stove->fBaffleTemp > P2F(sParams->sTemperature.fTarget)) || stove->bReloadRequested)
		{
			nextState = RELOAD_IGNITION;
			stove->TimeOfReloadRequest = u32CurrentTime_ms;
		}
	}

}
//** END: ZEROING STEPPER **//

//** STATE: RELOAD / IGNITION **//
static void Algo_reload_entry(Mobj* stove,const  PF_StateParam_t* sParams)
{
		stove->sPrimary.i8apertureSteps = sParams->sPrimary.i32Max;
		stove->sPrimary.fSecPerStep = 0; // force aperture
		stove->sGrill.i8apertureSteps = sParams->sGrill.i32Max;
		stove->sGrill.fSecPerStep = 0; // force aperture
		stove->sSecondary.i8apertureSteps = sParams->sSecondary.i32Max;
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
	stove->sPrimary.i8apertureSteps = sParams->sPrimary.i32Max;
	stove->sPrimary.fSecPerStep = 0; // force aperture
	stove->sGrill.i8apertureSteps = sParams->sGrill.i32Max;
	stove->sGrill.fSecPerStep = 0; // force aperture
	stove->sSecondary.i8apertureSteps = sParams->sSecondary.i32Max;
	stove->sSecondary.fSecPerStep = 0; // force aperture
	bStepperAdjustmentNeeded = true;
}

static void Algo_tempRise_action(Mobj* stove,const  PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms)
{
	static uint32_t u32TimeOfMajorCorr = 0;

	// Case(s) we want to wait
	if((u32TimeOfMajorCorr != 0 && (u32CurrentTime_ms - u32TimeOfMajorCorr < MINUTES(1))) || 	// We just made a correction
			(stove->fBaffleDeltaT < (P2F(sParams->sTempSlope.fTarget) - P2F(sParams->sTempSlope.fTolerance)))) // Temperature rises abnormally slow
	{
		return;
	}

	if(stove->fBaffleTemp >P2F( sParams->sTemperature.fTolerance)) // Here, fTolerance -> Temp to start regulating with particles
	{
		if((stove->sParticles->fparticles - stove->sParticles->fnormalized_zero) > (P2F(sParams->sParticles.fAbsMaxDiff)) &&
				stove->fBaffleDeltaT > (P2F(sParams->sTempSlope.fTarget) + P2F(sParams->sTempSlope.fTolerance)))
		{
			if(stove->sGrill.i8apertureSteps > 15)
			{
				stove->sGrill.i8apertureSteps /= 2;
				stove->sGrill.fSecPerStep = 0;
			}
			else if(stove->sGrill.i8apertureSteps > sParams->sGrill.i32Min)
			{
				stove->sGrill.i8apertureSteps = sParams->sGrill.i32Min;
				stove->sGrill.fSecPerStep = 0;
			}
			else
			{
				stove->sPrimary.i8apertureSteps = 75;
				stove->sPrimary.fSecPerStep = 0;
				nextState = stove->bThermostatOn ? COMBUSTION_HIGH : COMBUSTION_LOW;
			}


			bStepperAdjustmentNeeded = true;
			u32TimeOfMajorCorr = u32CurrentTime_ms;
			return;
		}
	}

	if(stove->sGrill.i8apertureSteps > sParams->sGrill.i32Min)
	{
		stove->sGrill.i8apertureSteps--;
		stove->sGrill.fSecPerStep = 6;
	}else
	{

		if(stove->sPrimary.i8apertureSteps-- <= 75)
		{
			nextState = stove->bThermostatOn ? COMBUSTION_HIGH : COMBUSTION_LOW;
		}
		stove->sPrimary.fSecPerStep = 6;
	}

	bStepperAdjustmentNeeded = true;

}
//** END: TEMPERATURE RISE **//


//** STATE: COMBUSTION LOW **//
static void Algo_combLow_entry(Mobj *stove,const  PF_StateParam_t* sParams)
{
	stove->sPrimary.i8apertureSteps = sParams->sPrimary.i32Max;
	stove->sPrimary.fSecPerStep = 0; // force aperture
	stove->sGrill.i8apertureSteps = sParams->sGrill.i32Max;
	stove->sGrill.fSecPerStep = 0; // force aperture
	stove->sSecondary.i8apertureSteps = sParams->sSecondary.i32Max;
	stove->sSecondary.fSecPerStep = 0; // force aperture
	bStepperAdjustmentNeeded = true;
}

static void Algo_combLow_action(Mobj* stove,const  PF_StateParam_t* sParams, uint32_t u32CurrentTime_ms)
{

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

	if(!sManParam->s32ManualOverride) // TODO: Put this in task function
	{
		nextState = lastState;
		return;
	}

	if(stove->sPrimary.i8apertureSteps != sManParam->s32ManualPrimary ||
			stove->sGrill.i8apertureSteps != sManParam->s32ManualGrill ||
			stove->sSecondary.i8apertureSteps != sManParam->s32ManualSecondary)
	{
		stove->sPrimary.i8apertureSteps = sManParam->s32ManualPrimary;
		stove->sPrimary.fSecPerStep = 0; // force aperture
		stove->sGrill.i8apertureSteps = sManParam->s32ManualGrill;
		stove->sGrill.fSecPerStep = 0; // force aperture
		stove->sSecondary.i8apertureSteps = sManParam->s32ManualSecondary;
		stove->sSecondary.fSecPerStep = 0; // force aperture
		bStepperAdjustmentNeeded = true;
	}


}

