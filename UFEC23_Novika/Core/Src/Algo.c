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


// Adjustment functions for each state
static fnComputeAdjustment AlgoComputeAdjustment[ALGO_NB_OF_STATE];
const PF_StateParam_t* sStateParams[ALGO_NB_OF_STATE];
static PF_StateParam_t sStatedummy;
const PF_OverHeat_Thresholds_t *sOverheatParams;

static void Algo_fill_compute_functions(void);
static void Algo_compute_reload(Mobj* stove, const PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms);
static void Algo_compute_Waiting(Mobj* stove, const PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms);
static void Algo_compute_tempRise(Mobj* stove, const PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms);
static void Algo_compute_combHigh(Mobj* stove, const PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms);
static void Algo_compute_combLow(Mobj* stove, const PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms);
static void Algo_compute_coalHigh(Mobj* stove, const PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms);
static void Algo_compute_coalLow(Mobj* stove, const PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms);
static void Algo_compute_manual(Mobj* stove, const PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms);
static void Algo_set_to_zero(Mobj* stove, const PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms);

void Algo_task(Mobj *stove, uint32_t u32CurrentTime_ms);
bool Algo_adjust_steppers_position(uint8_t* cmd);
void Algo_update_steppers_ready_flag(void);
void Algo_stoveInit(Mobj *stove);

void Algo_Init(void const * argument)
{
	static Mobj UFEC23;
	Algo_fill_compute_functions();

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

	if((AlgoComputeAdjustment[currentState] != NULL) && \
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
		stove->u32TimeOfStateEntry_ms = u32CurrentTime_ms;
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
	sStatedummy.i32EntryWaitTimeSeconds = 0;

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

void Algo_fill_compute_functions(void)
{
	AlgoComputeAdjustment[ZEROING_STEPPER] = Algo_set_to_zero;
	AlgoComputeAdjustment[WAITING] = Algo_compute_Waiting;
	AlgoComputeAdjustment[RELOAD_IGNITION] = Algo_compute_reload;
	AlgoComputeAdjustment[TEMPERATURE_RISE] = Algo_compute_tempRise;
	AlgoComputeAdjustment[COMBUSTION_HIGH] = Algo_compute_combHigh;
	AlgoComputeAdjustment[COMBUSTION_LOW] = Algo_compute_combLow;
	AlgoComputeAdjustment[COAL_LOW] = Algo_compute_coalLow;
	AlgoComputeAdjustment[COAL_HIGH] = Algo_compute_coalHigh;
	AlgoComputeAdjustment[OVERTEMP] = Algo_set_to_zero;
	AlgoComputeAdjustment[SAFETY] = Algo_set_to_zero;
	AlgoComputeAdjustment[MANUAL_CONTROL] = Algo_compute_manual;

}

static void Algo_set_to_zero(Mobj* stove,const  PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms)
{

	if(stove->bstateJustChanged)
	{
		for(uint8_t i = 0;i < NUMBER_OF_STEPPER_CMDS;i++)
		{
			cmd[i] = 0;
		}
		bStepperAdjustmentNeeded = true;
		stove->bstateJustChanged = false;
	}else if(currentState == ZEROING_STEPPER && motors_ready_for_req)
	{
		nextState = WAITING;
	}
}


static void Algo_compute_Waiting(Mobj* stove,const  PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms)
{
	if(stove->bstateJustChanged)
	{
		cmd[0] = sParams->sPrimary.i32Max;
		cmd[1] = 0; // force aperture
		cmd[2] = sParams->sGrill.i32Max;
		cmd[3] = 0; // force aperture
		cmd[4] = sParams->sSecondary.i32Max;
		cmd[5] = 0; // force aperture
		bStepperAdjustmentNeeded = true;
		stove->bstateJustChanged = false;
	}

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
static void Algo_compute_reload(Mobj* stove,const  PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms)
{
	if(stove->bstateJustChanged)
	{
		cmd[0] = sParams->sPrimary.i32Max;
		cmd[1] = 0; // force aperture
		cmd[2] = sParams->sGrill.i32Max;
		cmd[3] = 0; // force aperture
		cmd[4] = sParams->sSecondary.i32Max;
		cmd[5] = 0; // force aperture
		bStepperAdjustmentNeeded = true;
		stove->bstateJustChanged = false;
	}


	if((u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms > MINUTES(1) && (stove->fBaffleTemp > P2F(sParams->sTemperature.fTarget))) || stove->fBaffleTemp > 1000) // Hard coded... Not so important
	{
		nextState = TEMPERATURE_RISE;
	}

	if(u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms > MINUTES(20))
	{
		nextState = ZEROING_STEPPER;
	}
}

static void Algo_compute_tempRise(Mobj* stove,const  PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms)
{

	if(stove->bstateJustChanged)
	{
		cmd[0] = sParams->sPrimary.i32Max;
		cmd[1] = 0; // force aperture
		cmd[2] = sParams->sGrill.i32Max;
		cmd[3] = 0; // force aperture
		cmd[4] = sParams->sSecondary.i32Max;
		cmd[5] = 0; // force aperture
		bStepperAdjustmentNeeded = true;
		stove->bstateJustChanged = false;
	}

	if(u32CurrentTime_ms - stove->u32TimeOfStateEntry_ms > SECONDS(sParams->i32EntryWaitTimeSeconds))
	{

	}

}

static void Algo_compute_combHigh(Mobj* stove,const  PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms)
{

}

static void Algo_compute_combLow(Mobj* stove,const  PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms)
{

}

static void Algo_compute_coalHigh(Mobj* stove,const  PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms)
{

}

static void Algo_compute_coalLow(Mobj* stove,const  PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms)
{

}

static void Algo_compute_manual(Mobj* stove,const  PF_StateParam_t* sParams, uint8_t *cmd, uint32_t u32CurrentTime_ms)
{

}

