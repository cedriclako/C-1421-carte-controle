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

extern MessageBufferHandle_t MotorControlHanlde;

void vToggleOneStep(motor_t Motor);
void vDisableStepper(motor_t Motor);
void vEnableStepper(motor_t Motor);
bool bStepperZero(motor_t MotorId);
bool LimitSwitchActive(motor_t MotorId);
bool StepperAtSetpoint(StepObj * motor);
void StepperAdjustPosition(StepObj * motor, int16_t RequestedPosition);
void vSetStepperMotorDirection(motor_t Motor, motor_direction_t Direction);
void vStepperMaxTorque(motor_t Motor,bool bApplyMaxTorque);

//remapping of the pin for the current board
#define Primary_Step_SetHigh() HAL_GPIO_WritePin(Step1_STEP_GPIO_Port,Step1_STEP_Pin,GPIO_PIN_SET);
#define Grill_Step_SetHigh() HAL_GPIO_WritePin(Step2_STEP_GPIO_Port,Step2_STEP_Pin,GPIO_PIN_SET);
#define Secondary_Step_SetHigh() HAL_GPIO_WritePin(Step3_STEP_GPIO_Port,Step3_STEP_Pin,GPIO_PIN_SET);
#define Primary_Step_SetLow() HAL_GPIO_WritePin(Step1_STEP_GPIO_Port,Step1_STEP_Pin,GPIO_PIN_RESET);
#define Grill_Step_SetLow() HAL_GPIO_WritePin(Step2_STEP_GPIO_Port,Step2_STEP_Pin,GPIO_PIN_RESET);
#define Secondary_Step_SetLow() HAL_GPIO_WritePin(Step3_STEP_GPIO_Port,Step3_STEP_Pin,GPIO_PIN_RESET);
#define Primary_DISABLE() HAL_GPIO_WritePin(Step1_ENABLE_GPIO_Port,Step1_ENABLE_Pin,GPIO_PIN_SET);
#define Grill_DISABLE() HAL_GPIO_WritePin(Step2_ENABLE_GPIO_Port,Step2_ENABLE_Pin,GPIO_PIN_SET);
#define Secondary_DISABLE() HAL_GPIO_WritePin(Step3_ENABLE_GPIO_Port,Step3_ENABLE_Pin,GPIO_PIN_SET);
#define Primary_ENABLE() HAL_GPIO_WritePin(Step1_ENABLE_GPIO_Port,Step1_ENABLE_Pin,GPIO_PIN_RESET);
#define Grill_ENABLE() HAL_GPIO_WritePin(Step2_ENABLE_GPIO_Port,Step2_ENABLE_Pin,GPIO_PIN_RESET);
#define Secondary_ENABLE() HAL_GPIO_WritePin(Step3_ENABLE_GPIO_Port,Step3_ENABLE_Pin,GPIO_PIN_RESET);
#define Primary_RESET() HAL_GPIO_WritePin(Step1_RESET_GPIO_Port,Step1_RESET_Pin,GPIO_PIN_RESET);
#define Grill_RESET() HAL_GPIO_WritePin(Step2_RESET_GPIO_Port,Step2_RESET_Pin,GPIO_PIN_RESET);
#define Secondary_RESET() HAL_GPIO_WritePin(Step3_RESET_GPIO_Port,Step3_RESET_Pin,GPIO_PIN_RESET);
#define Primary_nRESET() HAL_GPIO_WritePin(Step1_RESET_GPIO_Port,Step1_RESET_Pin,GPIO_PIN_SET);
#define Grill_nRESET() HAL_GPIO_WritePin(Step2_RESET_GPIO_Port,Step2_RESET_Pin,GPIO_PIN_SET);
#define Secondary_nRESET() HAL_GPIO_WritePin(Step3_RESET_GPIO_Port,Step3_RESET_Pin,GPIO_PIN_SET);
#define Primary_TorqueMax() HAL_GPIO_WritePin(Step1_LowCurrent_GPIO_Port,Step1_LowCurrent_Pin,GPIO_PIN_RESET);
#define Grill_TorqueMax() HAL_GPIO_WritePin(Step2_LowCurrent_GPIO_Port,Step2_LowCurrent_Pin,GPIO_PIN_RESET);
#define Secondary_TorqueMax() HAL_GPIO_WritePin(Step3_LowCurrent_GPIO_Port,Step3_LowCurrent_Pin,GPIO_PIN_RESET);
#define Primary_TorqueMin() HAL_GPIO_WritePin(Step1_LowCurrent_GPIO_Port,Step1_LowCurrent_Pin,GPIO_PIN_SET);
#define Grill_TorqueMin() HAL_GPIO_WritePin(Step2_LowCurrent_GPIO_Port,Step2_LowCurrent_Pin,GPIO_PIN_SET);
#define Secondary_TorqueMin() HAL_GPIO_WritePin(Step3_LowCurrent_GPIO_Port,Step3_LowCurrent_Pin,GPIO_PIN_SET);
#define Primary_DIR_SetHigh() HAL_GPIO_WritePin(Step1_DIR_GPIO_Port,Step1_DIR_Pin,GPIO_PIN_SET);
#define Grill_DIR_SetHigh() HAL_GPIO_WritePin(Step2_DIR_GPIO_Port,Step2_DIR_Pin,GPIO_PIN_SET);
#define Secondary_DIR_SetHigh() HAL_GPIO_WritePin(Step3_DIR_GPIO_Port,Step3_DIR_Pin,GPIO_PIN_SET);
#define Primary_DIR_SetLow() HAL_GPIO_WritePin(Step1_DIR_GPIO_Port,Step1_DIR_Pin,GPIO_PIN_RESET);
#define Grill_DIR_SetLow() HAL_GPIO_WritePin(Step2_DIR_GPIO_Port,Step2_DIR_Pin,GPIO_PIN_RESET);
#define Secondary_DIR_SetLow() HAL_GPIO_WritePin(Step3_DIR_GPIO_Port,Step3_DIR_Pin,GPIO_PIN_RESET);

#define Step1_2_3_SLEEP() HAL_GPIO_WritePin(uc_Stepper_Sleep_GPIO_Port,uc_Stepper_Sleep_Pin,GPIO_PIN_SET);
#define Step1_2_3_WAKE() HAL_GPIO_WritePin(uc_Stepper_Sleep_GPIO_Port,uc_Stepper_Sleep_Pin,GPIO_PIN_RESET);


void Motor_Init(void const * argument)
{

	HAL_GPIO_WritePin(uc_Stepper_Sleep_GPIO_Port,uc_Stepper_Sleep_Pin,RESET);
	HAL_GPIO_WritePin(Stepper_HalfStep_GPIO_Port,Stepper_HalfStep_Pin,SET);

	StepObj motor[NumberOfMotors] = {
			STEPPER_INIT(PrimaryStepper,PF_PRIMARY_MINIMUM_OPENING,PF_PRIMARY_FULL_OPEN),
			STEPPER_INIT(GrillStepper,PF_GRILL_MINIMUM_OPENING,PF_GRILL_FULL_OPEN),
			STEPPER_INIT(SecondaryStepper,PF_SECONDARY_MINIMUM_OPENING,PF_SECONDARY_FULL_OPEN)
			};

	uint8_t u8cmd_buf[6] = {0x00};
	uint32_t u32CurrentTime_ms = 0;

  for(;;)
  {
	  u32CurrentTime_ms = osKernelSysTick();

	  if(StepperAtSetpoint(&motor[PrimaryStepper]) && StepperAtSetpoint(&motor[GrillStepper])
			  && StepperAtSetpoint(&motor[SecondaryStepper]))
	  {
		  if(xMessageBufferReceive(MotorControlHanlde, u8cmd_buf, 6, 10) == 6)
		  {
			  for(uint8_t i = 0;i < NumberOfMotors;i++)
			  {
				  motor[i].u8SetPoint = RANGE(motor->u8MinValue, u8cmd_buf[2*i], motor->u8MaxValue);
				  motor[i].fSecPerStep = (float) (u8cmd_buf[2*i + 1])/10;
			  }
		  }
	  }else
	  {
		  for(uint8_t i = 0;i < NumberOfMotors;i++)
		  {

			  if(!StepperAtSetpoint(&motor[i]))
			  {
				  if(motor[i].fSecPerStep == 0.0)
				  {
					  StepperAdjustPosition(&motor[i],motor[i].u8SetPoint);
				  }
				  if(u32CurrentTime_ms - motor[i].u32LastMove_ms > motor[i].fSecPerStep*1000)
				  {
					  StepperAdjustPosition(&motor[i],
							  motor[i].u8Position + (motor[i].u8SetPoint - motor[i].u8Position)/abs(motor[i].u8SetPoint - motor[i].u8Position));
					  motor[i].u32LastMove_ms = u32CurrentTime_ms;
				  }
			  }

		  }

	  }

	  osDelay(1);
  }


}

bool StepperAtSetpoint(StepObj * motor)
{
	return motor->u8Position == motor->u8SetPoint;
}


void StepperAdjustPosition(StepObj * motor, int16_t RequestedPosition)
{
	int16_t StepToPerform = 0;

    if(LimitSwitchActive((motor_t)motor->u8ID))
    {
    	motor->u8Position = motor->u8MinValue;
    	StepToPerform = motor->u8Position - RequestedPosition;
    }else if (motor->u8Position == motor->u8MinValue)
    {
    	StepToPerform = 1;
    }


    while (StepToPerform != 0)
    {
    	vEnableStepper((motor_t)motor->u8ID);
    	vStepperMaxTorque((motor_t)motor->u8ID,true);

    	if(StepToPerform > 0)
		{
			vSetStepperMotorDirection((motor_t)motor->u8ID, Closing);
			motor->u8Position = motor->u8Position - 1;

			if (motor->u8Position < motor->u8MinValue)
			{
				motor->u8Position = motor->u8MinValue;
				vDisableStepper((motor_t)motor->u8ID);
			}

 			vToggleOneStep((motor_t)motor->u8ID);
			StepToPerform--;
		}
		else if(StepToPerform < 0)
		{
			vEnableStepper((motor_t)motor->u8ID);
			osDelay(50);
			vSetStepperMotorDirection((motor_t)motor->u8ID, Opening);
			motor->u8Position = motor->u8Position + 1;
			vToggleOneStep((motor_t)motor->u8ID);
			StepToPerform = motor->u8Position - RequestedPosition;
		}
	}

}

void vToggleOneStep(motor_t Motor)
{
    switch(Motor)
    {
        case PrimaryStepper:
            Primary_Step_SetLow();
            osDelay(10);
            Primary_Step_SetHigh();
            osDelay(10);
            break;
        case GrillStepper:
            Grill_Step_SetLow();
            osDelay(10);
            Grill_Step_SetHigh();
            osDelay(10);
            break;
        case SecondaryStepper:
            Secondary_Step_SetLow();
            osDelay(10);
            Secondary_Step_SetHigh();
            osDelay(10);
            break;
        default:
            break;
    }
}
bool LimitSwitchActive(motor_t MotorId)
{
	bool active=false;
    switch(MotorId)
    {

    	case PrimaryStepper:
        	active = (GPIO_PIN_RESET == HAL_GPIO_ReadPin(Limit_switch1_GPIO_Port,Limit_switch1_Pin));
            break;

        case GrillStepper:
        	active = (GPIO_PIN_RESET == HAL_GPIO_ReadPin(Limit_switch2_GPIO_Port,Limit_switch2_Pin));
        	break;

        case SecondaryStepper:
        	active = (GPIO_PIN_RESET == HAL_GPIO_ReadPin(Limit_switch3_GPIO_Port,Limit_switch3_Pin));
        	break;

        default:
            break;
    }
    return active;
}

void vEnableStepper(motor_t Motor)
{
    Step1_2_3_WAKE();
	switch(Motor)
    {
        case PrimaryStepper:
            Primary_ENABLE();
            Primary_nRESET();
            break;
        case GrillStepper:
            Grill_ENABLE();
            Grill_nRESET();
            break;
        case SecondaryStepper:
        	Secondary_ENABLE();
        	Secondary_nRESET();

            break;
        default:
            break;
    }
}
void vStepperMaxTorque(motor_t Motor,bool bApplyMaxTorque)
{
	switch(Motor)
    {
        case PrimaryStepper:
        	if(bApplyMaxTorque)
        	{
        		Primary_TorqueMax();
        	}
        	else
        	{
        		Primary_TorqueMin();
        	}
            break;
        case GrillStepper:
        	if(bApplyMaxTorque)
        	{
        		Grill_TorqueMax();
        	}
        	else
			{
        		Grill_TorqueMin();
        	}
            break;
        case SecondaryStepper:
        	if(bApplyMaxTorque)
        	{
        		Secondary_TorqueMax();
        	}
        	else
			{
        		Secondary_TorqueMin();
        	}
            break;
        default:
            break;
    }
}

void vDisableStepper(motor_t Motor)
{
	//Step1_2_3_SLEEP();
    switch(Motor)
    {
        case PrimaryStepper:
            Primary_DISABLE();
            break;
        case GrillStepper:
            Grill_DISABLE();
            break;
        case SecondaryStepper:
        	Secondary_DISABLE();
        	break;
        default:
            break;
    }
}

void vSetStepperMotorDirection(motor_t Motor, motor_direction_t Direction)
{
    switch(Motor)
    {
    case PrimaryStepper:
        if(Direction == Opening)
        {
        	Primary_DIR_SetHigh();
        }
        else
        {
        	Primary_DIR_SetLow();
        }
        break;
    case GrillStepper:
        if(Direction == Opening)
        {
        	Grill_DIR_SetHigh();
        }
        else
        {
        	Grill_DIR_SetLow();
        }
        break;
    case SecondaryStepper:
        if(Direction == Opening)
        {
        	Secondary_DIR_SetHigh();
        }
        else
        {
        	Secondary_DIR_SetLow();
        }
        break;
    default:
    	while(1);
    	//wrong motor argument
    }

}

void StepperMotorProdTest(motor_t MotorId)
{
	// on ouvre jusqu'à ce que la limite switch soit inactive,
	// on ferme jusqu'à ce qu'elle soit active
	//on va au max et on ferme sur la switch
	//on ouvre à 25% et on désactive les moteurs.

	vSetStepperMotorDirection(MotorId, Opening);
	vEnableStepper(MotorId);
	while(LimitSwitchActive(MotorId))
	{
		vToggleOneStep(MotorId);
		osDelay(5);
	}
	vSetStepperMotorDirection(MotorId, Closing);
	while(!LimitSwitchActive(MotorId))
	{
		vToggleOneStep(MotorId);
		osDelay(5);
	}
	vSetStepperMotorDirection(MotorId, Opening);
	int stepToFull;
	stepToFull = (MotorId == PrimaryStepper)?STEP_RANGE_PRIMARY:STEP_RANGE_GRILL;
	int i;
	for (i=0;i<=stepToFull;i++)
	{
		vToggleOneStep(MotorId);
		osDelay(5);
	}
	vSetStepperMotorDirection(MotorId, Closing);
	for (i=0;i<=stepToFull;i++)
	{
		vToggleOneStep(MotorId);
		osDelay(5);
	}
	//repeat for proper 0
	vSetStepperMotorDirection(MotorId, Opening);
	while(LimitSwitchActive(MotorId))
	{
		vToggleOneStep(MotorId);
		osDelay(5);
	}
	vSetStepperMotorDirection(MotorId, Closing);
	while(!LimitSwitchActive(MotorId))
	{
		vToggleOneStep(MotorId);
		osDelay(5);
	}
	vSetStepperMotorDirection(MotorId, Opening);
	for (i=0;i<=stepToFull/3;i++)
	{
		vToggleOneStep(MotorId);
		osDelay(5);
	}
	vDisableStepper(MotorId);
}
void AllMotorToZero()
{
	int i=0;

	for(i=0;i<NumberOfMotors;i++)
	{
		vEnableStepper(i);
		vSetStepperMotorDirection(i, Opening);
		while(LimitSwitchActive(i))
		{
			vToggleOneStep(i);
			osDelay(5);
		}
		vSetStepperMotorDirection(i, Closing);
		while(!LimitSwitchActive(i))
		{
			vToggleOneStep(i);
			osDelay(5);
		}
		vDisableStepper(i);
	}
}

