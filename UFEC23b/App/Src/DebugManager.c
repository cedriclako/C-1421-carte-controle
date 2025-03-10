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
#include "FanManager.h"
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

extern RTC_HandleTypeDef hrtc;
RTC_TimeTypeDef sTime;

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

void PrintOutput(Mobj * stove, State currentState , State lastState , State nextState)
{

	HAL_RTC_GetTime(&hrtc,&sTime,0);
	printf("#");
	printf("%02i:%02i:%02i ",sTime.Hours, sTime.Minutes, sTime.Seconds);
	printf("Tbaffle:%i ", (int) stove->fBaffleTemp);
	printf("Tavant:%i ", (int) stove->fChamberTemp);
	printf("Plenum:%i ", (int) stove->fPlenumTemp);
	printf("State:");

	printf(ALGO_GetStateString(currentState));

	printf(" tStat:");
	if (tstat_status == true)
	{
		printf("ON ");
	}
	else
	{
		printf("OFF ");
	}
	printf("dTbaffle:%.1f ", stove->fBaffleDeltaT);
	printf("FanSpeed:");
	printf(Fan_GetSpeedString(FAN_DISTRIB));
	printf(" Grille:%i ", (int)(stove->sGrill.u8aperturePosSteps));
	printf("Prim:%i ", (int)(stove->sPrimary.u8aperturePosSteps));
	printf("Sec:%i ", (int)(stove->sSecondary.u8aperturePosSteps));
	printf("Tboard:%.0f ", get_BoardTemp());
	printf("Door:");

	if(stove->bDoorOpen == true)
	{
		printf("OPEN ");

	}
	else
	{
		printf("CLOSED ");
	}

	printf("PartCH0ON:%u ", stove->sParticles->u16ch0_ON);
	printf("PartCH1ON:%u ", stove->sParticles->u16ch1_ON);
	printf("PartCH0OFF:%u ", stove->sParticles->u16ch0_OFF);
	printf("PartCH1OFF:%u ", stove->sParticles->u16ch1_OFF);
	printf("PartVar:%u ", stove->sParticles->u16stDev);
	printf("PartSlope:%.1f ", stove->sParticles->fslope);
	printf("TPart:%u " ,stove->sParticles->u16temperature);
	printf("PartCurr:%.1f ", stove->sParticles->fLED_current_meas);
	printf("PartLuxON:%u ", stove->sParticles->u16Lux_ON);
	printf("PartLuxOFF:%u ", stove->sParticles->u16Lux_OFF);
	printf("PartTime:%lu ", stove->sParticles->u16TimeSinceInit);
	printf("dTavant: %.1f ", stove->fChamberDeltaT);



	if(print_debug_setup)
	{
		printf("Last State:");
		printf(ALGO_GetStateString(lastState));
		printf(" Next State:");
		printf(ALGO_GetStateString(nextState));
		printf(" Normalized Particles :%.2f",stove->sParticles->fparticles);
		printf(" Normalized Particles zero :%.2f",stove->sParticles->fnormalized_zero);




    if(get_motors_ready_status())
    {
      printf("\n\r motors_ready_for_req : ON ");
    }
    else
    {
      printf("\n\r motors_ready_for_req : OFF ");
    }



		if(stove->bSafetyOn == true)
		{
			printf("\n\rbSafety: On ");
		}
		else
		{
			printf("\n\rbSafety: Off ");
		}

		if(stove->bInterlockOn)
		{
			printf("\n\rbinterlock on \n");
		}
		else
		{
			printf("\n\rbinterlock off \n");
		}

		if(stove->bReloadRequested)
		{
			printf("\n\rbReloadRequested on \n");
		}
		else
		{
			printf("\n\rbReloadRequested off \n");
		}

			printf("\n\rfan 1 speed is : %i",Fan_GetSpeed(FAN_DISTRIB));
			printf("\n\rfan 2 speed is : %i",Fan_GetSpeed(FAN_BLOWER));

			printf("\n\rGrille_sec_per_steps:%i ", (stove->sGrill.fSecPerStep));
			printf("\n\rPrim_sec_per_steps:%i ", (stove->sPrimary.fSecPerStep));
			printf("\n\rSec_sec_per_steps:%i \n\r", (stove->sSecondary.fSecPerStep));


		if(stove->bstateJustChanged)
		{
		// print state that we entered parameters
		}





	}

	printf("*\n\r");
}

