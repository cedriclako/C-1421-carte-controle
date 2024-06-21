/*
 * FanManager.c
 *
 *  Created on: 26 sept. 2023
 *      Author: crichard
 */

#include "FanManager.h"
#include "stm32f1xx_hal.h"
#include "HelperMacro.h"
#include <stdlib.h>
#include "main.h"
#include "Algo.h"
#include <stdio.h>
#include <stdbool.h>

EXTI_HandleTypeDef hexti;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim5;

void Fan_EnableZeroDetect(void);
void Fan_DisableZeroDetect(void);
void Fan_DisableAll(void);
void Fan_DisableFan(FanObj *fan);
void Fan_EnableFan(FanObj *fan);
void Fan_ManageSpeed(FanObj *fan);

static const char* m_FanStrings[FSPEED_NUM_OF_SPEEDS] =
{
  HELPERMACRO_DEFSTRING(FSPEED_OFF),
  HELPERMACRO_DEFSTRING(FSPEED_LOW),
  HELPERMACRO_DEFSTRING(FSPEED_HIGH),
  HELPERMACRO_DEFSTRING(FSPEED_AUTO),


};

static FanObj sFans[FAN_NUM_OF_FANS] =
{
	FAN_INIT(PFD_RMT_DISTFAN, PFD_DIST_SPD, &htim4, &htim5, AFK_Speed1_Pin,AFK_Speed1_GPIO_Port), //Amélioration possible... Remapper sur timer 3 pour utilisation des fonctions OnePulse à délai
	FAN_INIT(PFD_RMT_LOWFAN, PFD_BLOW_SPD, &htim2, &htim3, FAN_SPEED3_Pin,FAN_SPEED3_GPIO_Port),

};

void Fan_Init(void)
{
	Fan_DisableZeroDetect();
}


void Fan_Process(Mobj *stove)
{
	const PF_UsrParam* uParam = PB_GetUserParam();
	static bool bHiTriggered = false;
	static bool bLoTriggered = false;


	if(stove->bDoorOpen)
	{
		Fan_DisableAll();
		return;
	}


	for(uint8_t i = 0; i < FAN_NUM_OF_FANS;i++)
	{
		sFans[i].eSpeed = (Fan_Speed_t) PARAMFILE_GetParamValueByKey(sFans[i].szSpeedKey);

		if((sFans[i].eSpeed == FSPEED_AUTO))
		{
			if(stove->fChamberTemp >= P2F(uParam->s32FAN_HI_KIP))
			{
				sFans[i].eSpeed = FSPEED_HIGH;
				// 2024-04-12 MC : enlevé l'overlap du fan high lorsqu'on a passé en dessous de KIP de fan high. Lorsqu'on est en dessous de fan hi kip, on retourne en low
				//bHiTriggered = true;
				//bLoTriggered = true;
			}
			else if(stove->fChamberTemp <= P2F(uParam->s32FAN_LO_KOP))
			{
				sFans[i].eSpeed = FSPEED_OFF;
				bLoTriggered = false;
				bHiTriggered = false;
			}


			else if(stove->fChamberTemp > P2F(uParam->s32FAN_LO_KIP))
			{
				if(bHiTriggered)
				{
					sFans[i].eSpeed = FSPEED_HIGH;
				}
				else
				{
					bLoTriggered = true;
					sFans[i].eSpeed = FSPEED_LOW;
				}
			}
			else
			{
				bHiTriggered = false;
				if(bLoTriggered)
				{
					sFans[i].eSpeed = FSPEED_LOW;
				}
				else
				{
					sFans[i].eSpeed = FSPEED_OFF;
				}
			}
		}


		if(sFans[i].bEnabled)
		{
			Fan_ManageSpeed(&sFans[i]);
		}
		else if((sFans[i].eSpeed != FSPEED_OFF))
		{
			sFans[i].bEnabled = true;
		}
	}
}


void Fan_ManageSpeed(FanObj *fan)
{

	if(fan->ePreviousSpeed != fan->eSpeed)
	{

		switch(fan->eSpeed)
		{
			case FSPEED_OFF:
				Fan_DisableFan(fan);
				fan->ePreviousSpeed = FSPEED_OFF;
				break;
			case FSPEED_LOW:

				fan->u16SpeedPercent = PARAMFILE_GetParamValueByKey(fan->szLowSpeedKey);
				fan->sStartTimer->Instance->ARR = (uint32_t) (8330 * ((float)(100 - fan->u16SpeedPercent)/100));
				fan->sStopTimer->Instance->ARR = (uint32_t) (3500 * ((float)(fan->u16SpeedPercent)/100));
				Fan_EnableZeroDetect();
				fan->ePreviousSpeed = FSPEED_LOW;
				break;
			case FSPEED_HIGH:

				fan->u16SpeedPercent = 100;
				HAL_GPIO_WritePin(fan->sPins.MODULATION_PORT,fan->sPins.MODULATION_PIN,GPIO_PIN_SET);
				fan->ePreviousSpeed = FSPEED_HIGH;
				break;
			default:
				break;
		}
	}

}

void Fan_DisableFan(FanObj *fan)
{

	fan->bEnabled = false;
	fan->ePreviousSpeed = FSPEED_OFF;
	HAL_GPIO_WritePin(fan->sPins.MODULATION_PORT,fan->sPins.MODULATION_PIN,GPIO_PIN_RESET);
}

void Fan_EnableFan(FanObj *fan)
{

	switch(fan->eSpeed)
	{
		case FSPEED_OFF:

			Fan_DisableFan(fan);
			break;
		case FSPEED_LOW:
			fan->bEnabled = true;
			break;
		case FSPEED_HIGH:

			fan->u16SpeedPercent = 100;
			fan->bEnabled = true;
			HAL_GPIO_WritePin(fan->sPins.MODULATION_PORT,fan->sPins.MODULATION_PIN,GPIO_PIN_SET);
			break;
		default:
			break;
	}
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == FAN_Zero_crossing_Pin)
	{
		if(sFans[FAN_BLOWER].bEnabled && sFans[FAN_BLOWER].u16SpeedPercent !=100)
		{
			HAL_TIM_Base_Start_IT(sFans[FAN_BLOWER].sStartTimer);
		}
		if(sFans[FAN_DISTRIB].bEnabled && sFans[FAN_DISTRIB].u16SpeedPercent !=100)
		{
			HAL_TIM_Base_Start_IT(sFans[FAN_DISTRIB].sStartTimer);
		}

	}

}

void Fan_StartPulseSPEED3(void)
{
	HAL_TIM_Base_Stop(sFans[FAN_BLOWER].sStartTimer);
	HAL_TIM_Base_Start_IT(sFans[FAN_BLOWER].sStopTimer);
	HAL_GPIO_WritePin(sFans[FAN_BLOWER].sPins.MODULATION_PORT,sFans[FAN_BLOWER].sPins.MODULATION_PIN,GPIO_PIN_SET);
}

void Fan_StopPulseSPEED3(void)
{
	HAL_TIM_Base_Stop(sFans[FAN_BLOWER].sStopTimer);

	if(sFans[FAN_BLOWER].u16SpeedPercent !=100)
	{
		HAL_GPIO_WritePin(sFans[FAN_BLOWER].sPins.MODULATION_PORT,sFans[FAN_BLOWER].sPins.MODULATION_PIN,GPIO_PIN_RESET);
	}

}

void Fan_StartPulseSPEED1(void)
{
	HAL_TIM_Base_Stop(sFans[FAN_DISTRIB].sStartTimer);
	HAL_TIM_Base_Start_IT(sFans[FAN_DISTRIB].sStopTimer);
	HAL_GPIO_WritePin(sFans[FAN_DISTRIB].sPins.MODULATION_PORT,sFans[FAN_DISTRIB].sPins.MODULATION_PIN,GPIO_PIN_SET);
}

void Fan_StopPulseSPEED1(void)
{
	HAL_TIM_Base_Stop(sFans[FAN_DISTRIB].sStopTimer);

	if(sFans[FAN_DISTRIB].u16SpeedPercent !=100)
	{
		HAL_GPIO_WritePin(sFans[FAN_DISTRIB].sPins.MODULATION_PORT,sFans[FAN_DISTRIB].sPins.MODULATION_PIN,GPIO_PIN_RESET);
	}

}


void Fan_EnableZeroDetect(void)
{
	HAL_NVIC_EnableIRQ(EXTI1_IRQn);
}

void Fan_DisableZeroDetect(void)
{
	HAL_NVIC_DisableIRQ(EXTI1_IRQn);
}

void Fan_DisableAll(void)
{
	for(uint8_t i = 0; i < FAN_NUM_OF_FANS;i++)
	{
		if(sFans[i].bEnabled)
		{
			Fan_DisableFan(&sFans[i]);
		}
	}

	Fan_DisableZeroDetect();
}

Fan_Speed_t Fan_GetSpeed(Fan_t FanID)
{
  return sFans[FanID].eSpeed;
}

const char* Fan_GetSpeedString(Fan_t FanID)
{
  if (sFans[FanID].eSpeed >= FSPEED_NUM_OF_SPEEDS)
    return "-- N/A --";
  return m_FanStrings[sFans[FanID].eSpeed];

}






