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
#include "cmsis_os.h"
#include "DebugPort.h"
#include "stm32f1xx_hal.h"
#include "TemperatureManager.h"
#include "MotorManager.h"
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include "main.h" //@@@@for the LED toggle to be removed
#include "algo.h"
#include <stdio.h>
#include "ParticlesManager.h"
#include "DebugManager.h"

static bool sec_input = false;

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function ---------------------------------------------------------*/

/* USER CODE BEGIN Header_Debugmanager */
/**
* @brief Function implementing the DebugManagerT thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Steppermanager */
void DebugManager(void const * argument)
{
  /* USER CODE BEGIN DebugManager */
  /* Infinite loop */

	State TempAlgoState;
	float* mod;

	for(;;)
	{


		osDelay(5000);
		HAL_RTC_GetTime(&hrtc,&sTime,0);
		printf("#");
		printf("%02i:%02i:%02i ",sTime.Hours,sTime.Minutes,sTime.Seconds);
		printf("Tbaffle:%i Tavant:%i Plenum:%i ",Algo_getBaffleTemp()/10,Algo_getFrontTemp()/10,Algo_getPlenumTemp()/10);
		printf("State:");

		TempAlgoState = Algo_getState();
		switch (TempAlgoState) {
			case ZEROING_STEPPER:
				printf("ZEROING_STEP");
				break;
			case WAITING:
				printf("WAITING");
				break;
			case TEMPERATURE_RISE:
				printf("TEMP_RISE");
				break;
			case COMBUSTION_LOW:
				printf("COMB_LOW");
				break;
			case COMBUSTION_SUPERLOW:
				printf("COMB_SUPERLOW");
				break;
			case COMBUSTION_HIGH:
				printf("COMB_HIGH");
				break;
			case RELOAD_IGNITION:
				printf("RELOAD_IGNI");
				break;
			case FLAME_LOSS:
				printf("FLAME_LOSS");
				break;
			case COAL_HIGH:
				printf("COAL_HIGH");
				break;
			case COAL_LOW:
				printf("COAL_LOW");
				break;
			case OVERTEMP:
				printf("OVERTEMP");
				break;
			case SAFETY:
				printf("SAFETY");
				break;
			case PRODUCTION_TEST:
				printf("PRODTEST");
				break;
			case MANUAL_CONTROL:
				printf("MANUAL");
				break;
			default:
				printf("UNKNOWN");
				break;
		}
		printf(" tStat:");
		if (Algo_getThermostatRequest())
		{
			printf("ON ");
		}
		else
		{
			printf("OFF ");
		}
		printf("dTbaffle:%f",Algo_getBaffleTempSlope());
		printf(" FanSpeed:%i ",Mot_getFanSpeed());
		printf("Grille:%i ",	Algo_getGrill()*9/10);
		printf("Prim:%i ",Algo_getPrimary()*9/10);
		printf("Sec:%i ",Algo_getSecondary()*9/10);
		printf("Tboard:%i ",get_BoardTemp());
		printf("Door:");
		if(IsDoorOpen())
		{
			printf("OPEN ");

		}
		else
		{
			printf("CLOSED ");
		}

		printf("PartCH0ON:%u ", Particle_getCH0());
		printf("PartCH1ON:%u ", Particle_getCH1());
		printf("PartCH0OFF:%u ",Particle_getCH0_OFF());
		printf("PartCH1OFF:%u ",Particle_getCH1_OFF());
		printf("PartVar:%u ",Particle_getVariance());
		printf("PartSlope:%i ",Particle_getSlope());
		printf("TPart:%u ",Particle_getTemperature());
		printf("PartCurr:%u ",Particle_getCurrent());
		printf("PartLuxON:%u ", Particle_getLuxON());
		printf("PartLuxOFF:%u ", Particle_getLuxOFF());
		printf("PartTime:%lu ", Particle_getTime());
		mod = get_algomod();
		printf("Crit:%f ",mod[2]);
		printf("AdjOffset:%f ",mod[0]);
		printf("SpeedDivider:%f ",mod[1]);
		printf("dTavant:%f",mod[3]);
		if(PM_isPboard_absent())
		{
			printf("GlobalStatus:PARTICLE_COMM_ERROR");
		}
		else
		{

		}
		printf("GlobalStatus: ");
		printf("*\n\r");
  }
  /* USER CODE END DebugManager */
}

void set_sec_flag(void)
{
	sec_input = true;
}
