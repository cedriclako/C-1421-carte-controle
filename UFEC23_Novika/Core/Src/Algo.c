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

#define NUMBER_OF_STEPPER_CMDS 6

extern MessageBufferHandle_t MotorControlsHandle;
extern QueueHandle_t MotorInPlaceHandle;
static bool motors_ready_for_req = true;
static bool bStepperAdjustmentNeeded = false;
static State currentState = ZEROING_STEPPER;
static State lastState = ZEROING_STEPPER;
static State nextState = ZEROING_STEPPER;


typedef void (*fnComputeAdjustment)(Mobj *stove,const PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms);
typedef void (*fnStateEntryAction)(Mobj *stove,const  PF_StateParam_t* sParams, uint8_t *cmd);

// Adjustment functions for each state
static fnComputeAdjustment AlgoComputeAdjustment[ALGO_NB_OF_STATE];
static fnStateEntryAction AlgoStateEntryAction[ALGO_NB_OF_STATE];
const PF_StateParam_t* sStateParams[ALGO_NB_OF_STATE];
static PF_StateParam_t sStatedummy;
const PF_OverHeat_Thresholds_t *sOverheatParams;

static void Algo_fill_state_functions(void);
static void Algo_reload_action(Mobj* stove, const PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms);
static void Algo_Waiting_action(Mobj* stove, const PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms);
static void Algo_tempRise_action(Mobj* stove, const PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms);
static void Algo_combHigh_action(Mobj* stove, const PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms);
static void Algo_combLow_action(Mobj* stove, const PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms);
static void Algo_coalHigh_action(Mobj* stove, const PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms);
static void Algo_coalLow_action(Mobj* stove, const PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms);
static void Algo_manual_action(Mobj* stove, const PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms);
static void Algo_zeroing_action(Mobj* stove, const PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms);

static void Algo_reload_entry(Mobj *stove,const  PF_StateParam_t* sParams, uint8_t *cmd);
static void Algo_zeroing_entry(Mobj *stove,const  PF_StateParam_t* sParams, uint8_t *cmd);
static void Algo_tempRise_entry(Mobj *stove,const  PF_StateParam_t* sParams, uint8_t *cmd);
static void Algo_combLow_entry(Mobj *stove,const  PF_StateParam_t* sParams, uint8_t *cmd);
static void Algo_combHigh_entry(Mobj *stove,const  PF_StateParam_t* sParams, uint8_t *cmd);
static void Algo_coalLow_entry(Mobj *stove,const  PF_StateParam_t* sParams, uint8_t *cmd);
static void Algo_coalHigh_entry(Mobj *stove,const  PF_StateParam_t* sParams, uint8_t *cmd);

void Algo_task(Mobj *stove, uint32_t u32CurrentTime_ms);
bool Algo_adjust_steppers_position(uint8_t* cmd);
void Algo_update_steppers_ready_flag(void);
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
	static uint8_t u8cmds[NUMBER_OF_STEPPER_CMDS] = {0x00};


	Algo_update_steppers_ready_flag();

	if(stove->bstateJustChanged)
	{
		stove->u32TimeOfStateEntry_ms = u32CurrentTime_ms;
		stove->bstateJustChanged = false;

		if(AlgoStateEntryAction[currentState] != NULL)
		{
			AlgoStateEntryAction[currentState](stove, sStateParams[currentState], u8cmds);
		}

	}
	else if((AlgoComputeAdjustment[currentState] != NULL) && \
			(u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms > SECONDS(sStateParams[currentState]->i32EntryWaitTimeSeconds)) && \
			(u32CurrentTime_ms - stove->u32TimeOfMotorRequest_ms > SECONDS(stove->u32TimeBeforeInPlace_ms)))
	{
		AlgoComputeAdjustment[currentState](stove, sStateParams[currentState], u8cmds, u32CurrentTime_ms);
	}


	if(motors_ready_for_req && bStepperAdjustmentNeeded)
	{
		if(Algo_adjust_steppers_position(u8cmds))
		{
			bStepperAdjustmentNeeded = false;
			stove->u32TimeOfMotorRequest_ms = u32CurrentTime_ms;
		}
	}

	if(nextState != currentState)
	{
		lastState = currentState;
		currentState = nextState;
		stove->bstateJustChanged = true;
	}

}


bool Algo_adjust_steppers_position(uint8_t* cmd)
{
	if(!motors_ready_for_req)
	{
		return false;
	}
	if(!xMessageBufferSend(MotorControlsHandle,cmd,6,0))
	{
		return false;
	}
	motors_ready_for_req = false;
	return true;
}

void Algo_update_steppers_ready_flag(void)
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
	stove->u32TimeOfMotorRequest_ms = 0;
	stove->u32TimeBeforeInPlace_ms = 0;
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
	//AlgoComputeAdjustment[OVERTEMP] = Algo_zeroing_action;
	//AlgoComputeAdjustment[SAFETY] = Algo_zeroing_action;
	AlgoComputeAdjustment[MANUAL_CONTROL] = Algo_manual_action;

	AlgoStateEntryAction[ZEROING_STEPPER] = Algo_zeroing_entry;

	AlgoStateEntryAction[RELOAD_IGNITION] = Algo_reload_entry;
	AlgoStateEntryAction[TEMPERATURE_RISE] = Algo_tempRise_entry;
	AlgoStateEntryAction[COMBUSTION_HIGH] = Algo_combHigh_entry;
	AlgoStateEntryAction[COMBUSTION_LOW] = Algo_combLow_entry;
	AlgoStateEntryAction[COAL_LOW] = Algo_coalLow_entry;
	AlgoStateEntryAction[COAL_HIGH] = Algo_coalHigh_entry;
	AlgoStateEntryAction[OVERTEMP] = Algo_zeroing_entry;
	AlgoStateEntryAction[SAFETY] = Algo_zeroing_entry;

}

static void Algo_zeroing_entry(Mobj *stove,const  PF_StateParam_t* sParams, uint8_t *cmd)
{
	for(uint8_t i = 0;i < NUMBER_OF_STEPPER_CMDS;i++)
	{
		cmd[i] = 0;
	}
	bStepperAdjustmentNeeded = true;
}

static void Algo_zeroing_action(Mobj* stove,const  PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms)
{

	if(motors_ready_for_req)
	{
		nextState = WAITING;
	}
}


static void Algo_Waiting_action(Mobj* stove,const  PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms)
{

	if((stove->fBaffleTemp > 800))
	{
		nextState = TEMPERATURE_RISE;
	}

	if(!stove->bInterlockOn)
	{
		if((stove->fBaffleTemp > P2F(sParams->sTemperature.fTarget)) || stove->bReloadRequested)
		{
			nextState = RELOAD_IGNITION;
			stove->TimeOfReloadRequest = u32CurrentTime_ms;
		}
	}

}

static void Algo_reload_entry(Mobj* stove,const  PF_StateParam_t* sParams, uint8_t *cmd)
{
		cmd[0] = sParams->sPrimary.i32Max;
		cmd[1] = 0; // force aperture
		cmd[2] = sParams->sGrill.i32Max;
		cmd[3] = 0; // force aperture
		cmd[4] = sParams->sSecondary.i32Max;
		cmd[5] = 0; // force aperture
		bStepperAdjustmentNeeded = true;

		if(stove->fBaffleTemp > 1000)
		{
			nextState = TEMPERATURE_RISE;
		}

}

static void Algo_reload_action(Mobj* stove,const  PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms)
{

	if((u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms > MINUTES(1) && (stove->fBaffleTemp > P2F(sParams->sTemperature.fTarget)))) // Hard coded... Not so important
	{
		nextState = TEMPERATURE_RISE;
	}

	if(u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms > MINUTES(20))
	{
		nextState = ZEROING_STEPPER;
	}
}

static void Algo_tempRise_entry(Mobj* stove,const  PF_StateParam_t* sParams, uint8_t *cmd)
{
	cmd[0] = sParams->sPrimary.i32Max;
	cmd[1] = 0; // force aperture
	cmd[2] = sParams->sGrill.i32Max;
	cmd[3] = 0; // force aperture
	cmd[4] = sParams->sSecondary.i32Max;
	cmd[5] = 0; // force aperture
	bStepperAdjustmentNeeded = true;
}

static void Algo_tempRise_action(Mobj* stove,const  PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms)
{


}

static void Algo_combLow_entry(Mobj *stove,const  PF_StateParam_t* sParams, uint8_t *cmd)
{

}

static void Algo_combLow_action(Mobj* stove,const  PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms)
{

}

static void Algo_combHigh_entry(Mobj *stove,const  PF_StateParam_t* sParams, uint8_t *cmd)
{

}


static void Algo_combHigh_action(Mobj* stove,const  PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms)
{

}

static void Algo_coalLow_entry(Mobj *stove,const  PF_StateParam_t* sParams, uint8_t *cmd)
{

}

static void Algo_coalLow_action(Mobj* stove,const  PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms)
{

}

static void Algo_coalHigh_entry(Mobj *stove,const  PF_StateParam_t* sParams, uint8_t *cmd)
{

}

static void Algo_coalHigh_action(Mobj* stove,const  PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms)
{

}


static void Algo_manual_action(Mobj* stove,const  PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms)
{
	const PF_UsrParam* sManParam = PB_GetUserParam();
	static uint8_t lastP = 0;
	static uint8_t lastG = 0;
	static uint8_t lastS = 0;

	if(!sManParam->s32ManualOverride)
	{
		nextState = lastState;
		return;
	}

	if((lastP != sManParam->s32ManualPrimary) || (lastG != sManParam->s32ManualGrill) || (lastS != sManParam->s32ManualSecondary))
	{
		lastP = sManParam->s32ManualPrimary;
		cmd[0] = lastP;
		cmd[1] = 0; // force aperture

		lastG = sManParam->s32ManualGrill;
		cmd[2] = lastG;
		cmd[3] = 0; // force aperture

		lastS = sManParam->s32ManualSecondary;
		cmd[4] = lastS;
		cmd[5] = 0; // force aperture
		bStepperAdjustmentNeeded = true;
	}



}

