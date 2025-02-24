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

#include <MotorManager.h>
#include "main.h"
#include "ParamFile.h"
#include "cmsis_os.h"
#include "task.h"
#include "algo.h"
#include "DebugPort.h"
#include <stdio.h>

static MotorControl_t stMotor[NumberOfMotors];

void vToggleOneStep(motor_t Motor);
void vDisableStepper(motor_t Motor);
void vEnableStepper(motor_t Motor);
bool bStepperZero(motor_t MotorId);
bool vLimitSwitchActive(motor_t MotorId);
void vSetStepperMotorDirection(motor_t Motor, motor_direction_t Direction);
void vStepperPositioning(int RequestedPosition,int *CurrentPosition, motor_t MotorId);
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

//#define limitSwitchGrillActive() (GPIO_PIN_RESET == HAL_GPIO_ReadPin(LimitSwith1_GPIO_Port,LimitSwith1_Pin))
//#define limitSwitchPrimaryActive() (GPIO_PIN_RESET == HAL_GPIO_ReadPin(LimitSwitch2_GPIO_Port,LimitSwitch2_Pin))

//KIP KOP Temperature for fan operation.
static const FanSpeedKipKopParam_t FanSpeedParameters =
{
	.KipSpeed1 = 1300,
	.KopSpeed1 = 1050,
	.KipSpeed2 = 6000,
	.KopSpeed2 = 3500,
	.KipSpeed3 = 1500,
	.KopSpeed3 = 1450,
	.KipSpeed4 = 1600,
	.KopSpeed4 = 1550,
	.NoDemandKipSpeed1 = 1900,
	.NoDemandKopSpeed1 = 1650,
	.NoDemandKopSpeed2 = 1950,
	.NoDemandKipSpeed2 = 1850,
	.NoDemandKipSpeed3 = 2000,
	.NoDemandKopSpeed3 = 1900,
	.NoDemandKipSpeed4 = 2100,
	.NoDemandKopSpeed4 = 1950
};


//private variables
static Mot_FanSpeed plenumSpeed = MOT_PLENUM_STOP;

static int stepperPosition[NumberOfMotors];
//static int GrillPosition = 0;
//static int PrimaryPosition = 0;
//static int SecondaryPosition = 0;



bool Mot_InPosition( AirInput * self,motor_t Motorid)
{
	return self->aperture == stepperPosition[Motorid];
}

/* USER CODE BEGIN Header_Steppermanager */
/**
* @brief Function implementing the StepperManagerT thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Steppermanager */
void Steppermanager(void const * argument)
{
  /* USER CODE BEGIN Steppermanager */
  /* Infinite loop */
	//sleep is active low but we inverse the logic with transistor
	//static int GrillPosition = 0;
	//static int PrimaryPosition = 0;
	//printf("\n Stepper manager running");

	HAL_GPIO_WritePin(uc_Stepper_Sleep_GPIO_Port,uc_Stepper_Sleep_Pin,RESET);
	HAL_GPIO_WritePin(Stepper_HalfStep_GPIO_Port,Stepper_HalfStep_Pin,SET);
	Algo_init();

  for(;;)
  {
//#ifdef TEST_MOTEUR
//	vStepperPositioning(tman_getMoteur1Req(),&GrillPosition,GrillStepper);
//	vStepperPositioning(tman_getMoteur2Req(),&PrimaryPosition,PrimaryStepper);
//#else
	osDelay(50);

	Algo_task(osKernelSysTick());
	if(Algo_getState() != PRODUCTION_TEST)
	{
		vStepperPositioning(Algo_getGrill(),&stepperPosition[GrillStepper],GrillStepper);
		vStepperPositioning(Algo_getPrimary(),&stepperPosition[PrimaryStepper],PrimaryStepper);
		vStepperPositioning(Algo_getSecondary(),&stepperPosition[SecondaryStepper],SecondaryStepper);
	}

//#endif
  }
  /* USER CODE END Steppermanager */
}


void vStepperPositioning(int RequestedPosition, int *CurrentPosition, motor_t MotorId)
{
    //MotorControl_t* pstMotorControl;
    //pstMotorControl = &stMotor[MotorId];
    int StepToPerform = 0;

    bool StepperToZero = false;

    StepperToZero = vLimitSwitchActive(MotorId);

    if(StepperToZero)
    {
    	switch(MotorId)
    	{
    	case PrimaryStepper:
    		*CurrentPosition = (int)PF_PRIMARY_MINIMUM_OPENING;
    		break;
    	case GrillStepper:
    		*CurrentPosition = (int)PF_GRILL_MINIMUM_OPENING;
    		break;
    	case SecondaryStepper:
    		*CurrentPosition = (int)PF_SECONDARY_MINIMUM_OPENING;
    		break;
    	default:
    		break;
    	}
    }

    StepToPerform = *CurrentPosition - RequestedPosition;

	switch(MotorId)
	{
	case PrimaryStepper:
		if (*CurrentPosition == (int)PF_PRIMARY_MINIMUM_OPENING	 && !StepperToZero)
		{
			StepToPerform = 1;
		}
		break;
	case GrillStepper:
		if (*CurrentPosition == (int)PF_GRILL_MINIMUM_OPENING && !StepperToZero)
		{
			StepToPerform = 1;
		}
		break;
	case SecondaryStepper:
		if (*CurrentPosition == (int)PF_SECONDARY_MINIMUM_OPENING && !StepperToZero)
		{
			StepToPerform = 1;
		}
		break;
	default:
		break;
	}

    while (StepToPerform != 0)
    {
    	vEnableStepper(MotorId);
    	vStepperMaxTorque(MotorId,true);

    	if(StepToPerform > 0)
		{
			vSetStepperMotorDirection(MotorId, Closing);
			*CurrentPosition = *CurrentPosition - 1;

			switch(MotorId)
			{
			case PrimaryStepper:
				if (*CurrentPosition < (int)PF_PRIMARY_MINIMUM_OPENING)
				{
					*CurrentPosition = (int)PF_PRIMARY_MINIMUM_OPENING;
					vDisableStepper(PrimaryStepper);
				}
				break;
			case GrillStepper:
				if(*CurrentPosition < (int)PF_GRILL_MINIMUM_OPENING)
				{
					*CurrentPosition = (int)PF_GRILL_MINIMUM_OPENING;
					vDisableStepper(GrillStepper);
				}
				break;
			case SecondaryStepper:
				if (*CurrentPosition < (int)PF_SECONDARY_MINIMUM_OPENING)
				{
					*CurrentPosition = (int)PF_SECONDARY_MINIMUM_OPENING;
					vDisableStepper(SecondaryStepper);
				}
				break;
			default:
				break;
			}

 			vToggleOneStep(MotorId);
			StepToPerform--;
		}
		else if(StepToPerform < 0)
		{
			vEnableStepper(MotorId);
			osDelay(50);
			vSetStepperMotorDirection(MotorId, Opening);
			*CurrentPosition = *CurrentPosition + 1;
			vToggleOneStep(MotorId);
			StepToPerform = *CurrentPosition - RequestedPosition;
		}
		//Calculate or new position
	}
	//vStepperMaxTorque(MotorId,false);
    //vStepperMaxTorque(MotorId,true); TODO : re-enable max torque (Charles Richard) Semble fonctionner tel quel, MC
    //osDelay(100);
	//vDisableStepper(MotorId);

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
bool vLimitSwitchActive(motor_t MotorId)
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

void mot_SetStepMotor(motor_t MotorId, uint8_t ui8Opening)
{
    stMotor[MotorId].i8PositionRequested = ui8Opening;
}

MotorControl_t* pstGetMotor(motor_t Index)
{
    return &stMotor[Index];
}

void vSetSpeed(Mot_FanSpeed RequestedSpeed)
{
	  switch (RequestedSpeed)
	  {
		  case 1://speed1
			  HAL_GPIO_WritePin(SPEED2_COIL_GPIO_Port,SPEED2_COIL_Pin,RESET);
			  HAL_GPIO_WritePin(SPEED3_COIL_GPIO_Port,SPEED3_COIL_Pin,RESET);
			  osDelay(200);//break before make
			  //HAL_GPIO_WritePin(SPEED1_COIL_GPIO_Port,SPEED1_COIL_Pin,SET);
			  break;
		  case 2://speed2
			  HAL_GPIO_WritePin(SPEED3_COIL_GPIO_Port,SPEED3_COIL_Pin,RESET);
			  //HAL_GPIO_WritePin(SPEED1_COIL_GPIO_Port,SPEED1_COIL_Pin,RESET);
			  osDelay(200);//break before make
			  HAL_GPIO_WritePin(SPEED2_COIL_GPIO_Port,SPEED2_COIL_Pin,SET);
			  break;
		  case 3://speed3
			  //HAL_GPIO_WritePin(SPEED1_COIL_GPIO_Port,SPEED1_COIL_Pin,RESET);
			  HAL_GPIO_WritePin(SPEED2_COIL_GPIO_Port,SPEED2_COIL_Pin,RESET);
			  osDelay(200);//break before make
			  HAL_GPIO_WritePin(SPEED3_COIL_GPIO_Port,SPEED3_COIL_Pin,SET);
			  break;
		  case 4: //speed4 controlled by hardware
		  default: //stop
			  //HAL_GPIO_WritePin(SPEED1_COIL_GPIO_Port,SPEED1_COIL_Pin,RESET);
			  HAL_GPIO_WritePin(SPEED2_COIL_GPIO_Port,SPEED2_COIL_Pin,RESET);
			  HAL_GPIO_WritePin(SPEED3_COIL_GPIO_Port,SPEED3_COIL_Pin,RESET);
			  break;
	  }
}

Mot_FanSpeed Mot_getFanSpeed() {
  return plenumSpeed;
}

void manageFans(int baffleTemp, const PF_UsrParam* Param)
{
	if(IsDoorOpen())
	{
		HAL_GPIO_WritePin(SPEED2_COIL_GPIO_Port,SPEED2_COIL_Pin,RESET);
		if(baffleTemp < Param->s32FAN_KOP)
		{
			HAL_GPIO_WritePin(SPEED3_COIL_GPIO_Port,SPEED3_COIL_Pin,RESET);
		}
		return;
	}
	if(baffleTemp > Param->s32FAN_KIP)
	{
		  HAL_GPIO_WritePin(SPEED2_COIL_GPIO_Port,SPEED2_COIL_Pin,SET);
		  HAL_GPIO_WritePin(SPEED3_COIL_GPIO_Port,SPEED3_COIL_Pin,SET);
	}else if(baffleTemp < Param->s32FAN_KOP)
	{
		  HAL_GPIO_WritePin(SPEED2_COIL_GPIO_Port,SPEED2_COIL_Pin,RESET);
		  HAL_GPIO_WritePin(SPEED3_COIL_GPIO_Port,SPEED3_COIL_Pin,RESET);
	}
}

void managePlenumSpeed(int plenumTemp, bool thermostatRequest,uint32_t Time_ms) {

	static Mot_FanSpeed plenumPreviousSpeed = MOT_PLENUM_STOP;
	static uint32_t TimeSinceReloadRequest = 0;
	uint32_t TimeOfReloadRequest;


    int tempstate = Algo_getState();
    TimeOfReloadRequest = Algo_getTimeOfReloadRequest();
    TimeSinceReloadRequest = Time_ms - TimeOfReloadRequest;

	if(tempstate == RELOAD_IGNITION && fanPauseRequired)
	{
		if(TimeSinceReloadRequest > MINUTES(2))
		{
			fanPauseRequired = false;
		}
		else
		{
			plenumSpeed = MOT_PLENUM_STOP;
		}
	}
	else
	{
	  if(thermostatRequest) {
		  if(plenumTemp <= FanSpeedParameters.KopSpeed1 || (plenumTemp < FanSpeedParameters.KipSpeed1 && plenumPreviousSpeed == MOT_PLENUM_STOP))
		  {
			plenumSpeed = MOT_PLENUM_STOP;
		  }
		  else
		  {
			plenumSpeed = MOT_PLENUM_LOW;
		  }
	  }
	  else {
		plenumSpeed = (plenumTemp >= FanSpeedParameters.NoDemandKopSpeed1 && plenumPreviousSpeed == MOT_PLENUM_LOW) || plenumTemp >= FanSpeedParameters.NoDemandKipSpeed1 ? MOT_PLENUM_LOW : MOT_PLENUM_STOP;
	  }
	}

//	  if (tempstate == RELOAD_IGNITION && (plenumTemp <= (FanSpeedParameters.NoDemandKipSpeed1-100)))
//	  {
//		  //overwrite when in reload to avoid smoke if door is open unless temperature above 165F.
//		  plenumSpeed = MOT_PLENUM_STOP;
//	  }
	  if(tempstate == PRODUCTION_TEST)
	  {
		  //do nothing handle in test prod
	  }
	  else
	  {
		  vSetSpeed(plenumSpeed);
	  }
  plenumPreviousSpeed = plenumSpeed;
}

void StepperMotorProdTest(motor_t MotorId)
{
	// on ouvre jusqu'à ce que la limite switch soit inactive,
	// on ferme jusqu'à ce qu'elle soit active
	//on va au max et on ferme sur la switch
	//on ouvre à 25% et on désactive les moteurs.

	vSetStepperMotorDirection(MotorId, Opening);
	vEnableStepper(MotorId);
	while(vLimitSwitchActive(MotorId))
	{
		vToggleOneStep(MotorId);
		osDelay(5);
	}
	vSetStepperMotorDirection(MotorId, Closing);
	while(!vLimitSwitchActive(MotorId))
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
	while(vLimitSwitchActive(MotorId))
	{
		vToggleOneStep(MotorId);
		osDelay(5);
	}
	vSetStepperMotorDirection(MotorId, Closing);
	while(!vLimitSwitchActive(MotorId))
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
		while(vLimitSwitchActive(i))
		{
			vToggleOneStep(i);
			osDelay(5);
		}
		vSetStepperMotorDirection(i, Closing);
		while(!vLimitSwitchActive(i))
		{
			vToggleOneStep(i);
			osDelay(5);
		}
		vDisableStepper(i);
	}
}


