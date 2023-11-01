/*
 * FanManager.c
 *
 *  Created on: 26 sept. 2023
 *      Author: crichard
 */

#include "FanManager.h"
#include "stm32f1xx_hal.h"
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
int fan1speed;
int fan2speed;


//static FAN_states eFANstate = FANSTATE_IDLE;

static FanObj sFans[FAN_NUM_OF_FANS] =
{
	FAN_INIT(PFD_RMT_DISTFAN, PFD_AFK_SPD, &htim4, &htim5, AFK_Speed1_Pin,AFK_Speed1_GPIO_Port), //Amélioration possible... Remapper sur timer 3 pour utilisation des fonctions OnePulse à délai
	FAN_INIT(PFD_RMT_LOWFAN, PFD_FANL_SPD, &htim2, &htim3, FAN_SPEED3_Pin,FAN_SPEED3_GPIO_Port),

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


	fan1speed = sFans[0].eSpeed;
	fan2speed = sFans[1].eSpeed;


	for(uint8_t i = 0; i < FAN_NUM_OF_FANS;i++)
	{
		sFans[i].eSpeed = (Fan_Speed_t) PARAMFILE_GetParamValueByKey(sFans[i].szSpeedKey);

		if((sFans[i].eSpeed == FSPEED_AUTO))
		{
			if(stove->fBaffleTemp >= P2F(uParam->s32FAN_HI_KIP))
			{
				sFans[i].eSpeed = FSPEED_HIGH;
				bHiTriggered = true;
				bLoTriggered = true;
			}
			else if(stove->fBaffleTemp <= P2F(uParam->s32FAN_LO_KOP))
			{
				sFans[i].eSpeed = FSPEED_OFF;
				bLoTriggered = false;
				bHiTriggered = false;
			}
			else if(stove->fBaffleTemp > P2F(uParam->s32FAN_LO_KIP))
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
		if(sFans[FAN_FAN_L].bEnabled && sFans[FAN_FAN_L].u16SpeedPercent !=100)
		{
			HAL_TIM_Base_Start_IT(sFans[FAN_FAN_L].sStartTimer);
		}
		if(sFans[FAN_AFK].bEnabled && sFans[FAN_AFK].u16SpeedPercent !=100)
		{
			HAL_TIM_Base_Start_IT(sFans[FAN_AFK].sStartTimer);
		}

	}

}

void Fan_StartPulseSPEED3(void)
{
	HAL_TIM_Base_Stop(sFans[FAN_FAN_L].sStartTimer);
	HAL_TIM_Base_Start_IT(sFans[FAN_FAN_L].sStopTimer);
	HAL_GPIO_WritePin(sFans[FAN_FAN_L].sPins.MODULATION_PORT,sFans[FAN_FAN_L].sPins.MODULATION_PIN,GPIO_PIN_SET);
}

void Fan_StopPulseSPEED3(void)
{
	HAL_TIM_Base_Stop(sFans[FAN_FAN_L].sStopTimer);

	if(sFans[FAN_FAN_L].u16SpeedPercent !=100)
	{
		HAL_GPIO_WritePin(sFans[FAN_FAN_L].sPins.MODULATION_PORT,sFans[FAN_FAN_L].sPins.MODULATION_PIN,GPIO_PIN_RESET);
	}

}

void Fan_StartPulseSPEED1(void)
{
	HAL_TIM_Base_Stop(sFans[FAN_AFK].sStartTimer);
	HAL_TIM_Base_Start_IT(sFans[FAN_AFK].sStopTimer);
	HAL_GPIO_WritePin(sFans[FAN_AFK].sPins.MODULATION_PORT,sFans[FAN_AFK].sPins.MODULATION_PIN,GPIO_PIN_SET);
}

void Fan_StopPulseSPEED1(void)
{
	HAL_TIM_Base_Stop(sFans[FAN_AFK].sStopTimer);

	if(sFans[FAN_AFK].u16SpeedPercent !=100)
	{
		HAL_GPIO_WritePin(sFans[FAN_AFK].sPins.MODULATION_PORT,sFans[FAN_AFK].sPins.MODULATION_PIN,GPIO_PIN_RESET);
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





