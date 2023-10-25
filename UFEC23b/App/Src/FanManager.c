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

typedef enum
{
	FANSTATE_IDLE,
	FANSTATE_AUTO_RUN,
	FANSTATE_MANUAL_RUN,


	FANSTATE_NUM_OF_STATES

}FAN_states;

EXTI_HandleTypeDef hexti;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim5;

void Fan_EnableZeroDetect(void);
void Fan_DisableZeroDetect(void);
void Fan_DisableAll(void);
void Fan_DisableFan(FanObj *fan);
void Fan_EnableAuto(FanObj *fan);
void Fan_ManualOperation(Fan_t FanID, Mobj *stove);
void Fan_ManageSpeed(FanObj *fan);

static FAN_states eFANstate = FANSTATE_IDLE;

static FanObj sFans[FAN_NUM_OF_FANS] =
{
	FAN_INIT(60,90,100,PFD_AFK_SPD, &htim4, &htim5, AFK_Speed1_Pin,AFK_Speed1_GPIO_Port),
	FAN_INIT(60,90,100,PFD_FANL_SPD, &htim2, &htim3, FAN_SPEED3_Pin,FAN_SPEED3_GPIO_Port),

};

void Fan_Init(void)
{
	Fan_DisableZeroDetect();
}


void Fan_Process(Mobj *stove)
{
	const PF_UsrParam* uParam = PB_GetUserParam();
	bool bAtLeastOneFanEnabled = false;
	uint16_t u16tmp;

	switch(eFANstate)
	{
	case FANSTATE_IDLE:
		if((stove->fBaffleTemp > P2F(uParam->s32FAN_KIP)) && !stove->bDoorOpen)
		{
			for(uint8_t i = 0; i < FAN_NUM_OF_FANS;i++)
			{
				Fan_EnableAuto(&sFans[i]);
			}

		}

		break;
	case FANSTATE_AUTO_RUN:

		if(stove->bDoorOpen || (stove->fBaffleTemp < P2F(uParam->s32FAN_KOP)))
		{
			Fan_DisableAll();
			return;
		}

		for(uint8_t i = 0; i < FAN_NUM_OF_FANS;i++)
		{
			if(sFans[i].bEnabled)
			{
				int32_t s32Value = 0;
				PFL_GetValueInt32(&PARAMFILE_g_sHandle, sFans[i].szSpeedKey, &s32Value);
				u16tmp = (uint16_t)s32Value;
				if(sFans[i].u16SpeedPercent != u16tmp)
				{
					sFans[i].u16SpeedPercent = u16tmp;
					Fan_ManageSpeed(&sFans[i]);
				}

				bAtLeastOneFanEnabled = true;
			}
		}

		if(!bAtLeastOneFanEnabled)
		{
			eFANstate = FANSTATE_IDLE;
		}

		break;
	case FANSTATE_MANUAL_RUN:
		if(stove->bDoorOpen)
		{
			Fan_DisableAll();
			return;
		}

		for(uint8_t i = 0; i < FAN_NUM_OF_FANS;i++)
		{
			Fan_ManualOperation(i,stove);
		}

		break;
	default:
		break;
	}
	//else if((stove->fBaffleTemp < P2F(uParam->s32FAN_KOP)) )//Add flag for manual fan control here
	//{
	//	Fan_DisableAll();
	//	return;
	//}

void Fan_ManualOperation(Fan_t FanID, Mobj *stove)
{

	switch(sFans[FanID].eSpeed)
	{
	case FSPEED_OFF:
		if(sFans[FanID].bEnabled)
		{
			Fan_DisableFan(&sFans[FanID]);
		}
		break;
	case FSPEED_LOW:
		if(!sFans[FanID].bEnabled)
		{

			Fan_EnableZeroDetect();
			sFans[FanID].u16SpeedPercent = sFans[FanID].u16LowSpeedPercent;
			Fan_ManageSpeed(&sFans[FanID]);
			sFans[FanID].bEnabled = true;
		}
		break;
	case FSPEED_MED:
		if(!sFans[FanID].bEnabled)
		{
			Fan_EnableZeroDetect();
			sFans[FanID].u16SpeedPercent = sFans[FanID].u16MedSpeedPercent;
			Fan_ManageSpeed(&sFans[FanID]);
			sFans[FanID].bEnabled = true;
		}

		break;
	case FSPEED_HIGH:
		if(!sFans[FanID].bEnabled)
		{
			Fan_EnableZeroDetect();
			sFans[FanID].u16SpeedPercent = sFans[FanID].u16HighSpeedPercent;
			Fan_ManageSpeed(&sFans[FanID]);
			sFans[FanID].bEnabled = true;
		}

		break;
	default:
		break;

	}

}

void Fan_SetToManual(void)
{
	eFANstate = FANSTATE_MANUAL_RUN;
}

void Fan_SetOutOfManual(void)
{
	eFANstate = FANSTATE_IDLE;
}

void Fan_ManageSpeed(FanObj *fan)
{

	if(fan->ePreviousSpeed != fan->eSpeed)
	{
		HAL_GPIO_WritePin(fan->sPins.MODULATION_PORT,fan->sPins.MODULATION_PIN,GPIO_PIN_SET);
		return;
	}

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

void Fan_EnableAuto(FanObj *fan)
{
	int32_t s32Value = 0;
	PFL_GetValueInt32(&PARAMFILE_g_sHandle, fan->szSpeedKey, &s32Value);
	fan->u16SpeedPercent = (uint16_t)s32Value;

	if(fan->u16SpeedPercent == 100)
	{
		HAL_GPIO_WritePin(fan->sPins.MODULATION_PORT,fan->sPins.MODULATION_PIN,GPIO_PIN_SET);
	}
	else
	{
		fan->sStartTimer->Instance->ARR = (uint32_t) (8330 * ((float)(100 - fan->u16SpeedPercent)/100));
		fan->sStopTimer->Instance->ARR = (uint32_t) (3500 * ((float)(fan->u16SpeedPercent)/100));
		Fan_EnableZeroDetect();
	}

	fan->bEnabled = true;

	eFANstate = FANSTATE_AUTO_RUN;
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
	eFANstate = FANSTATE_IDLE;
}





