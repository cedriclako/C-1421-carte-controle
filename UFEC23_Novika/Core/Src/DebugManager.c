/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : DebugManager.c
  * @brief          : Print debug message to serial Port
  ******************************************************************************
  * @attention
  *
  * NOTICE:  All information contained herein is, and remains
  * the property of SBI.  The intellectual and technical concepts contained
  * herein are proprietary to SBI may be covered by Canadian, US and Foreign Patents,
  * patents in process, and are protected by trade secret or copyright law.
  * Dissemination of this information or reproduction of this material
  * is strictly forbidden unless prior written permission is obtained
  * from SBI.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/

/* Private includes ----------------------------------------------------------*/
#include "DebugPort.h"
#include "stm32f1xx_hal.h"
#include "TemperatureManager.h"
#include "DebugManager.h"
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "Algo.h"


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function ---------------------------------------------------------*/


void DebugManager(Mobj * stove, uint32_t u32time_ms)
{
	static uint32_t u32LastTimeInDebug = 0;

	if(u32time_ms - u32LastTimeInDebug > SECONDS(5))
	{
		//printf("%.2f\r\n",stove->fBaffleTemp);
		//printf("%.2f\r\n",stove->fBaffleDeltaT);
		//printf("%.2f\r\n",stove->fChamberTemp);
		//printf("%.2f\r\n",stove->fPlenumTemp);

		u32LastTimeInDebug = u32time_ms;
	}
}

const char* StateStrings[ALGO_NB_OF_STATE] =
{
		"ZEROING_STEP",
		"WAITING",
		"RELOAD_IGNI",
		"TEMP_RISE",
		"COMB_HIGH",
		"COMB_LOW",
		"COAL_LOW",
		"COAL_HIGH",
		"OVERTEMP",
		"SAFETY",
		"MANUAL"
};

void PrintOutput(Mobj * stove, State currentState)
{

	//HAL_RTC_GetTime(&hrtc,&sTime,0);
	printf("#");
	//printf("%02i:%02i:%02i ",sTime.Hours,sTime.Minutes,sTime.Seconds);
	printf("\tTbaffle: %i",(int) stove->fBaffleTemp);
	printf("\tdTbaffle: %.1f",stove->fBaffleDeltaT);
	printf("\tTavant: %i",(int) stove->fChamberTemp);
	printf("\tdTavant: %.1f",stove->fChamberDeltaT);
	printf("\tPlenum: %i ",(int) stove->fPlenumTemp);
	printf("State:");

	printf(StateStrings[currentState]);

	//printf(" tStat:");
	//if (stove->bThermostatOn)
	//{
	//	printf("ON ");
	//}
	//else
	//{
	//	printf("OFF ");
	//}

	//printf(" FanSpeed:%i ",0);
	//printf("Grille:%i ",	stove->sGrill.i8apertureSteps*9/10);
	//printf("Prim:%i ",stove->sPrimary.i8apertureSteps*9/10);
	printf("\tSec: %i ",stove->sSecondary.i8apertureSteps*9/10);
	//printf("Tboard:%.0f ",get_BoardTemp());
	//printf("Door:");
	//if(1)
	//{
	//	printf("OPEN ");

	//}
	//else
	//{
	//	printf("CLOSED ");
	//}

	printf("\tPartCH0ON: %u ", stove->sParticles->u16ch0_ON);
	//printf("PartCH1ON:%u ", stove->sParticles->u16ch1_ON);
	printf("\tPartCH0OFF: %u ",stove->sParticles->u16ch0_OFF);
	//printf("PartCH1OFF:%u ",stove->sParticles->u16ch1_OFF);
	printf("\tPartVar: %u ",stove->sParticles->u16stDev);
	printf("\tPartSlope: %.1f ",stove->sParticles->fslope);
	printf("\tTPart:%u ",stove->sParticles->u16temperature);
	printf("\tPartCurr: %.1f ",stove->sParticles->fLED_current_meas);
	//printf("PartLuxON:%u ", stove->sParticles->u16Lux_ON);
	//printf("PartLuxOFF:%u ", stove->sParticles->u16Lux_OFF);
	//printf("PartTime:%lu ", stove->sParticles->u16TimeSinceInit);

	printf("*\n\r\n\r");
}

