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
//#include "ProdTest.h"
//#include "MotorManager.h"
//#include "ParticlesManager.h"
#include "EspBridge.h"
#include "TemperatureManager.h"
#include "DebugManager.h"
#include "Algo.h"


static State currentState = ZEROING_STEPPER;
static uint32_t timeSinceStateEntry;
static uint32_t TimeOfReloadRequest;

void Algo_Init(void const * argument)
{
	static Mobj UFEC23;
	const PF_MotorOpeningsParam_t* pGrillMotorParam = PB_GetGrillMotorParam();
	const PF_MotorOpeningsParam_t* pPrimaryMotorParam = PB_GetPrimaryMotorParam();
	const PF_MotorOpeningsParam_t* pSecondaryMotorParam = PB_GetSecondaryMotorParam();
	const PF_CombTempParam_t* pTemperatureParam = PB_GetTemperatureParam();
	const PF_UsrParam* pUserParam = PB_GetUserParam();

	Temperature_Init();
	PARAMFILE_Init();
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
    	//TemperatureManager(&UFEC23,osKernelSysTick());
    	DebugManager(&UFEC23,osKernelSysTick());
    	//ESPMANAGER_Task();
    	ParticlesManager(osKernelSysTick());
    	osDelay(10);
    }

}

void Algo_task(void)
{

}

