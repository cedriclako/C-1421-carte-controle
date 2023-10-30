/******************************************************************************
  * File Name          : StepperManager.c
  * Description        : Code for Controlling motor position
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 SBI inc.
  * All rights reserved.</center></h2>
  *

  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/

#include "MotorManager.h"
#include "cmsis_os.h"
#include "message_buffer.h"
#include "stm32f1xx_hal.h"
#include <stdlib.h>
#include "main.h"
#include <stdio.h>


#define Step1_2_3_SLEEP() HAL_GPIO_WritePin(uc_Stepper_Sleep_GPIO_Port,uc_Stepper_Sleep_Pin,GPIO_PIN_SET);
#define Step1_2_3_WAKE() HAL_GPIO_WritePin(uc_Stepper_Sleep_GPIO_Port,uc_Stepper_Sleep_Pin,GPIO_PIN_RESET);

#define STEP_PERIOD 10 // ms per step

extern MessageBufferHandle_t MotorControlsHandle;
extern QueueHandle_t MotorInPlaceHandle;

void StepperAdjustPosition(StepObj *motor);
void StepperRecoverPosition(StepObj *motor);
bool StepperHome(StepObj *motor);

void StepperEnable(StepObj * motor);
void StepperDisable(StepObj * motor);
void StepperToggleOneStep(StepObj * motor);
void StepperSetDirection(StepObj *motor);
void StepperLowCurrentON(StepObj *motor);
void StepperLowCurrentOFF(StepObj *motor);

bool StepperAtSetpoint(StepObj *motor);
bool StepperLimitSwitchActive(StepObj *motor);

void Motor_task(void const * argument)
{

	Step1_2_3_WAKE();

	StepObj motor[NumberOfMotors] = {
			STEPPER_INIT(PF_PRIMARY_MINIMUM_OPENING,PF_PRIMARY_FULL_OPEN, PF_PRIMARY_REST_POSITION,Closing, Step1_STEP_Pin, Step1_ENABLE_Pin, Step1_RESET_Pin, Step1_LowCurrent_Pin, Step1_DIR_Pin, Limit_switch1_Pin,
					Step1_STEP_GPIO_Port, Step1_ENABLE_GPIO_Port, Step1_RESET_GPIO_Port, Step1_LowCurrent_GPIO_Port, Step1_DIR_GPIO_Port, Limit_switch1_GPIO_Port),
			STEPPER_INIT(PF_GRILL_MINIMUM_OPENING,PF_GRILL_FULL_OPEN, PF_GRILL_REST_POSITION,Closing, Step2_STEP_Pin,Step2_ENABLE_Pin,Step2_RESET_Pin,Step2_LowCurrent_Pin,Step2_DIR_Pin, Limit_switch2_Pin,
								Step2_STEP_GPIO_Port, Step2_ENABLE_GPIO_Port, Step2_RESET_GPIO_Port,Step2_LowCurrent_GPIO_Port,Step2_DIR_GPIO_Port, Limit_switch2_GPIO_Port),
#if NOVIKA_SETUP
			STEPPER_INIT(PF_SECONDARY_MINIMUM_OPENING,PF_SECONDARY_FULL_OPEN, PF_SECONDARY_REST_POSITION, Closing, Step3_STEP_Pin,Step3_ENABLE_Pin,Step3_RESET_Pin,Step3_LowCurrent_Pin,Step3_DIR_Pin, Limit_switch3_Pin,
					Step3_STEP_GPIO_Port,Step3_ENABLE_GPIO_Port,Step3_RESET_GPIO_Port,Step3_LowCurrent_GPIO_Port,Step3_DIR_GPIO_Port, Limit_switch3_GPIO_Port),
#else
			STEPPER_INIT(PF_SECONDARY_MINIMUM_OPENING,PF_SECONDARY_FULL_OPEN, PF_SECONDARY_REST_POSITION, Opening, Step3_STEP_Pin,Step3_ENABLE_Pin,Step3_RESET_Pin,Step3_LowCurrent_Pin,Step3_DIR_Pin, Limit_switch3_Pin,
					Step3_STEP_GPIO_Port,Step3_ENABLE_GPIO_Port,Step3_RESET_GPIO_Port,Step3_LowCurrent_GPIO_Port,Step3_DIR_GPIO_Port, Limit_switch3_GPIO_Port),
#endif

			};

	bool AllInPlace = true;
	uint8_t u8cmd_buf[6] = {0x00};
	uint32_t u32CurrentTime_ms = 0;

  for(;;)
  {
	  u32CurrentTime_ms = osKernelSysTick();

	  if(xMessageBufferIsFull(MotorControlsHandle) == pdTRUE)
	  {
		  xMessageBufferReceive(MotorControlsHandle, u8cmd_buf, 6, 5);

		  for(uint8_t i = 0;i < NumberOfMotors;i++)
		  {
			  if(u8cmd_buf[2*i] == MOTOR_HOME_CMD)
			  {
				  motor[i].bHomingRequest = true;
				  motor[i].u8SetPoint = motor[i].u8HomePosition;
				  motor[i].u8Position = 100;
			  }
			  else
			  {

			  motor[i].u8SetPoint = RANGE(motor[i].u8MinValue, u8cmd_buf[2*i], motor[i].u8MaxValue);
			  motor[i].fSecPerStep = (float) (u8cmd_buf[2*i + 1])/10;
			  }

		  }

	  }


	  for(uint8_t i = 0;i < NumberOfMotors;i++)
	  {
		  if(!motor[i].bHomingRequest && !motor[i].bRecovering && StepperLimitSwitchActive(&motor[i]) && (abs(motor[i].u8Position - motor[i].u8HomePosition) > 6) && (abs(motor[i].u8SetPoint - motor[i].u8HomePosition) > 6))
		  {
			  motor[i].bRecovering = true;
			  motor[i].u8RecoverPosition = motor[i].u8Position;
			  motor[i].u8Position = motor[i].u8HomePosition;
		  }
	  }





	  if(StepperAtSetpoint(&motor[PrimaryStepper]) && StepperAtSetpoint(&motor[GrillStepper])
			  && StepperAtSetpoint(&motor[SecondaryStepper]))
	  {
		  if(!AllInPlace)
		  {
			  AllInPlace = true;
			  xQueueSend(MotorInPlaceHandle,&AllInPlace,0);
		  }
		  osDelay(1);
	  }else
	  {
		  AllInPlace = false;
		  for(uint8_t i = 0;i < NumberOfMotors;i++)
		  {
			  if(!StepperAtSetpoint(&motor[i]) && ((u32CurrentTime_ms - motor[i].u32LastMove_ms) > STEP_PERIOD))
			  {
				  if(motor[i].bInMiddleOfStep)
				  {
					  StepperToggleOneStep(&motor[i]);

				  }
				  else if(motor[i].bHomingRequest)
				  {
					  if(StepperHome(&motor[i]))
					  {
						  motor[i].bHomingRequest = false;
					  }
				  }
				  else if(motor[i].bRecovering)
				  {
					  StepperRecoverPosition(&motor[i]);
				  }
				  // this is where we're using motor speed adjustments
				  else if((u32CurrentTime_ms - motor[i].u32LastMove_ms )> motor[i].fSecPerStep*1000) // TODO: valider si ça se passe correctement
				  {
					  StepperAdjustPosition(&motor[i]);
				  }
			  }
		  }

	  }

	  //osDelay(1);
  }


}

bool StepperAtSetpoint(StepObj *motor)
{
	return motor->u8Position == motor->u8SetPoint;
}


bool StepperHome(StepObj *motor)
{
	if(!StepperLimitSwitchActive(motor))
	{
		StepperEnable(motor);
		StepperLowCurrentON(motor);
		if(motor->u8HomePosition > 50)
		{
			motor->sDirection = Opening;
		}
		else
		{
			motor->sDirection = Closing;
		}
	    StepperSetDirection(motor);

	    StepperToggleOneStep(motor);
	    return false;

	}
	StepperDisable(motor);
	motor->u8Position = motor->u8HomePosition;
	return true;
}

void StepperRecoverPosition(StepObj *motor)
{
	int8_t delta_pos;

	StepperEnable(motor);
	StepperLowCurrentOFF(motor);

	if(motor->u8RecoverPosition > motor->u8HomePosition)
	{
		motor->sDirection = Opening;
		StepperSetDirection(motor);
		delta_pos = 1;
	}
	else
	{
		motor->sDirection = Closing;
		StepperSetDirection(motor);
		delta_pos = -1;
	}


	motor->u8Position += delta_pos;
	StepperToggleOneStep(motor);


	if(motor->u8Position == motor->u8RecoverPosition)
	{
		motor->bRecovering = false;
		StepperLowCurrentON(motor);
	}

}

void StepperAdjustPosition(StepObj *motor)
{

	int8_t delta_pos;

    if(StepperLimitSwitchActive(motor))
    {
    	motor->u8Position = motor->u8HomePosition;
    }

	StepperEnable(motor);
	StepperLowCurrentOFF(motor);

	if(motor->u8Position > motor->u8SetPoint)
	{
		motor->sDirection = Closing;
		StepperSetDirection(motor);
		delta_pos = -1;
	}
	else
	{
		motor->sDirection = Opening;
		StepperSetDirection(motor);
		delta_pos = 1;
	}


    motor->u8Position += delta_pos;
	StepperToggleOneStep(motor);

    if(StepperLimitSwitchActive(motor) && (motor->sDirection == motor->sHomingDirection))
	{
		motor->u8Position = motor->u8HomePosition; // On a atteint le minimum, on peut désactiver le moteur
		StepperDisable(motor);
	}
    else if(motor->u8Position == motor->u8HomePosition) // On pense qu'on est au minimum, mais on est perdu
    {
    		motor->bHomingRequest = true;
    		motor->u8SetPoint = motor->u8HomePosition;
    		motor->u8Position = 100;
    }

    if(motor->u8Position == motor->u8SetPoint)
    {
    	StepperLowCurrentON(motor); // To remove if spring load is too strong (reduces torque)
    }

}

void StepperToggleOneStep(StepObj * motor)
{

	if(!motor->bInMiddleOfStep)
	{
		HAL_GPIO_WritePin(motor->sPins.PWM_PORT,motor->sPins.PWM_PIN,GPIO_PIN_RESET);
		motor->bInMiddleOfStep = true;
	}
	else
	{
		HAL_GPIO_WritePin(motor->sPins.PWM_PORT,motor->sPins.PWM_PIN,GPIO_PIN_SET);
		motor->bInMiddleOfStep = false;
	}

	motor->u32LastMove_ms = osKernelSysTick(); // TODO: Valider si ça se passe correctement
}
bool StepperLimitSwitchActive(StepObj * motor)
{

	return HAL_GPIO_ReadPin(motor->sPins.SWITCH_PORT,motor->sPins.SWITCH_PIN) == GPIO_PIN_RESET;
}

void StepperEnable(StepObj * motor)
{
    HAL_GPIO_WritePin(motor->sPins.ENABLE_PORT,motor->sPins.ENABLE_PIN,GPIO_PIN_RESET);
    HAL_GPIO_WritePin(motor->sPins.RESET_PORT,motor->sPins.RESET_PIN,GPIO_PIN_SET);
}

void StepperLowCurrentOFF(StepObj *motor)
{
	HAL_GPIO_WritePin(motor->sPins.LOW_CURRENT_PORT,motor->sPins.LOW_CURRENT_PIN,GPIO_PIN_RESET);
}

void StepperLowCurrentON(StepObj *motor)
{
	HAL_GPIO_WritePin(motor->sPins.LOW_CURRENT_PORT,motor->sPins.LOW_CURRENT_PIN,GPIO_PIN_SET);
}

void StepperDisable(StepObj *motor)
{
    HAL_GPIO_WritePin(motor->sPins.ENABLE_PORT,motor->sPins.ENABLE_PIN,GPIO_PIN_SET);
}

void StepperSetDirection(StepObj *motor)
{
	HAL_GPIO_WritePin(motor->sPins.DIRECTION_PORT,motor->sPins.DIRECTION_PIN,motor->sDirection);
}



