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
		printf("%.2f\r\n",stove->fBaffleTemp);
		printf("%.2f\r\n",stove->fBaffleDeltaT);
		//printf("%.2f\r\n",stove->fChamberTemp);
		//printf("%.2f\r\n",stove->fPlenumTemp);
		u32LastTimeInDebug = u32time_ms;
	}
}

