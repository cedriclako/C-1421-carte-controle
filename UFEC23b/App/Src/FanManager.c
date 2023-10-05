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

#define FAN_PULSE_WIDTH		1  // in ms

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

static bool bZeroJustCrossed;
static uint32_t u32ZeroCrossTime_ms = 0;

void Fan_EnableZeroDetect(void);
void Fan_DisableZeroDetect(void);
void Fan_DisableAll(void);
void Fan_DisableFan(FanObj *fan);
void Fan_EnableAuto(FanObj *fan);
void Fan_ManualOperation(Fan_t FanID, Mobj *stove, uint32_t u32CurrentTime_ms);
void Fan_ManageSpeed(FanObj *fan, Mobj *stove, uint32_t u32CurrentTime_ms);
void Fan_GeneratePulse(FanObj *fan, uint32_t u32CurrentTime_ms);


static FAN_states eFANstate = FANSTATE_IDLE;

static FanObj sFans[FAN_NUM_OF_FANS] =
{
	FAN_INIT(60,90,100,PFD_AFK_SPD, AFK_Speed1_Pin,AFK_Speed1_GPIO_Port),
	FAN_INIT(60,90,100,PFD_FANL_SPD, FAN_SPEED3_Pin,FAN_SPEED3_GPIO_Port),

};

void Fan_Init(void)
{
	Fan_DisableZeroDetect();
	bZeroJustCrossed = false;

}


void Fan_Process(Mobj *stove, uint32_t u32CurrentTime_ms)
{
	const PF_UsrParam* uParam = PB_GetUserParam();
	bool bAtLeastOneFanEnabled = false;

	if(bZeroJustCrossed)
	{
		bZeroJustCrossed = false;
		u32ZeroCrossTime_ms = u32CurrentTime_ms;
	}

	uint32_t dummy = 0;

	switch(eFANstate)
	{
	case FANSTATE_IDLE:
		if((stove->fBaffleTemp > P2F(uParam->s32FAN_KIP)) && !stove->bDoorOpen)
		{
			for(uint8_t i = 0; i < FAN_NUM_OF_FANS-1;i++)
			{
				Fan_EnableAuto(&sFans[i]);
			}

			dummy = (uint32_t) (8330 * ((float)(100 - uParam->s32FANL_SPD)/100));
			htim2.Instance->ARR = dummy;
			htim3.Instance->ARR = (uint32_t) (3500 * ((float)(uParam->s32FANL_SPD)/100));

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
				sFans[i].u16SpeedPercent = PARAMFILE_GetParamValueByKey(sFans[i].szSpeedKey);
				Fan_ManageSpeed(&sFans[i],stove,u32CurrentTime_ms);
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
			Fan_ManualOperation(i,stove,u32CurrentTime_ms);
		}

		break;
	default:
		break;
	}
}

void Fan_ManualOperation(Fan_t FanID, Mobj *stove, uint32_t u32CurrentTime_ms)
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
			sFans[FanID].bEnabled = true;
			Fan_EnableZeroDetect();
		}
		else
		{
			sFans[FanID].u16SpeedPercent = sFans[FanID].u16LowSpeedPercent;
			Fan_ManageSpeed(&sFans[FanID],stove,u32CurrentTime_ms);
		}
		break;
	case FSPEED_MED:
		if(!sFans[FanID].bEnabled)
		{
			sFans[FanID].bEnabled = true;
			Fan_EnableZeroDetect();
		}
		else
		{
			sFans[FanID].u16SpeedPercent = sFans[FanID].u16MedSpeedPercent;
			Fan_ManageSpeed(&sFans[FanID],stove,u32CurrentTime_ms);
		}
		break;
	case FSPEED_HIGH:
		if(!sFans[FanID].bEnabled)
		{
			sFans[FanID].bEnabled = true;
			Fan_EnableZeroDetect();
		}
		else
		{
			sFans[FanID].u16SpeedPercent = sFans[FanID].u16HighSpeedPercent;
			Fan_ManageSpeed(&sFans[FanID],stove,u32CurrentTime_ms);
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

void Fan_ManageSpeed(FanObj *fan, Mobj *stove, uint32_t u32CurrentTime_ms)
{

	if(fan->u16SpeedPercent == 100)
	{
		HAL_GPIO_WritePin(fan->sPins.MODULATION_PORT,fan->sPins.MODULATION_PIN,GPIO_PIN_SET);
		return;
	}

	if(fan->u32PulseStart_ms != 0)
	{
		if((u32CurrentTime_ms - fan->u32PulseStart_ms) > FAN_PULSE_WIDTH)
		{
			HAL_GPIO_WritePin(fan->sPins.MODULATION_PORT,fan->sPins.MODULATION_PIN,GPIO_PIN_RESET);
			fan->u32PulseStart_ms = 0;
		}
	}
	else
	{
		if((u32CurrentTime_ms - u32ZeroCrossTime_ms > ((100-fan->u16SpeedPercent)*8.33/100)) && (u32CurrentTime_ms - u32ZeroCrossTime_ms < 8.33))
		{
			Fan_GeneratePulse(fan, u32CurrentTime_ms);

		}
	}

}

void Fan_DisableFan(FanObj *fan)
{

	fan->bEnabled = false;
	HAL_GPIO_WritePin(fan->sPins.MODULATION_PORT,fan->sPins.MODULATION_PIN,GPIO_PIN_RESET);
}

void Fan_EnableAuto(FanObj *fan)
{
	fan->bEnabled = true;
	Fan_EnableZeroDetect();

	fan->u16SpeedPercent = PARAMFILE_GetParamValueByKey(fan->szSpeedKey);

	if(fan->u16SpeedPercent == 100)
	{
		HAL_GPIO_WritePin(fan->sPins.MODULATION_PORT,fan->sPins.MODULATION_PIN,GPIO_PIN_SET);
	}

	eFANstate = FANSTATE_AUTO_RUN;
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == FAN_Zero_crossing_Pin)
	{
		bZeroJustCrossed = true;
		HAL_TIM_Base_Start_IT(&htim2);
	}

}

void Fan_StartPulseSPEED3(void)
{
	//HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
	//HAL_TIM_OnePulse_Start_IT(&htim3, TIM_CHANNEL_2);
	HAL_TIM_Base_Stop(&htim2);
	HAL_TIM_Base_Start_IT(&htim3);
	HAL_GPIO_WritePin(sFans[FAN_FAN_L].sPins.MODULATION_PORT,sFans[FAN_FAN_L].sPins.MODULATION_PIN,GPIO_PIN_SET);
}

void Fan_StopPulseSPEED3(void)
{
	HAL_TIM_Base_Stop(&htim3);
	HAL_GPIO_WritePin(sFans[FAN_FAN_L].sPins.MODULATION_PORT,sFans[FAN_FAN_L].sPins.MODULATION_PIN,GPIO_PIN_RESET);
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

void Fan_GeneratePulse(FanObj *fan, uint32_t u32CurrentTime_ms)
{
	HAL_GPIO_WritePin(fan->sPins.MODULATION_PORT,fan->sPins.MODULATION_PIN,GPIO_PIN_SET);

	fan->u32PulseStart_ms = u32CurrentTime_ms;
}




