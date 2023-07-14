/*
 * Algo.c
 *
 *  Created on: Jul 12, 2023
 *      Author: crichard
 */

#include "ParamFile.h"
#include "cmsis_os.h"
#include "main.h"
//#include "DebugPort.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
//#include "ProdTest.h"
//#include "MotorManager.h"
//#include "ParticlesManager.h"
#include "TemperatureManager.h"
#include "DebugManager.h"
#include "Algo.h"


static State currentState = ZEROING_STEPPER;
static uint32_t timeSinceStateEntry;
static uint32_t TimeOfReloadRequest;

void Algo_Init(void const * argument)
{
	static Mobj UFEC23;

	Temperature_Init();

    for(;;)
    {
    	TemperatureManager(&UFEC23,osKernelSysTick());
    	DebugManager(&UFEC23,osKernelSysTick());
    	osDelay(100);
    }

}

void Algo_task(void)
{

}

