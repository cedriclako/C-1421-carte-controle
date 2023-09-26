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
	FANSTATE_RUNNING,


	FANSTATE_NUM_OF_STATES

}FAN_states;

EXTI_HandleTypeDef hexti;

static bool bZeroJustCrossed;

//void Fan_EnableZeroDetect(void);
void Fan_DisableZeroDetect(void);
void Fan_DisableAll(void);
void FanDisableFan(FanObj *fan);
void FanEnableFan(FanObj *fan, uint16_t u16Speed);
void Fan_ManageSpeed(FanObj *fan, Mobj *stove, uint32_t u32CurrentTime_ms);


static FAN_states eFANstate;
static FanObj sFans[FAN_NUM_OF_FANS] =
{
	FAN_INIT(60,100,AFK_Speed1_Pin,AFK_Speed1_GPIO_Port),
	FAN_INIT(60,100,FAN_SPEED3_Pin,FAN_SPEED3_GPIO_Port),

};

void Fan_Init(void)
{
	Fan_DisableZeroDetect();
}


void Fan_Process(Mobj *stove, uint32_t u32CurrentTime_ms)
{

	switch(eFANstate)
	{
	case FANSTATE_IDLE:
		break;
	case FANSTATE_RUNNING:

		if(stove->bDoorOpen)
		{
			Fan_DisableAll();
			return;
		}

		for(uint8_t i = 0; i < FAN_NUM_OF_FANS;i++)
		{
			if(sFans[i].bEnabled)
			{
				Fan_ManageSpeed(&sFans[i],stove,u32CurrentTime_ms);
			}
		}

		break;
	default:
		break;
	}
}

void Fan_ManageSpeed(FanObj *fan, Mobj *stove, uint32_t u32CurrentTime_ms)
{


}

void FanDisableFan(FanObj *fan)
{

	fan->bEnabled = false;
	HAL_GPIO_WritePin(fan->sPins.MODULATION_PORT,fan->sPins.MODULATION_PIN,GPIO_PIN_RESET);
}

void FanEnableFan(FanObj *fan, uint16_t u16Speed)
{
	fan->bEnabled = true;

	fan->u16FanSpeedPercent = RANGE(fan->u16MinSpeedPercent, u16Speed, fan->u16MaxSpeedPercent);
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == FAN_Zero_crossing_Pin)
	{
		bZeroJustCrossed = true;
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
			FanDisableFan(&sFans[i]);
		}
	}

	Fan_DisableZeroDetect();
	eFANstate = FANSTATE_IDLE;
}


