/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : ProdTest.c
  * @brief          : ProductionTest Runner
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
#include "cmsis_os.h"
#include "stm32f1xx_hal.h"
#include "MotorManager.h"
#include "algo.h"
#include "main.h"
#include "ProdTest.h"
#include "Hmi.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function ---------------------------------------------------------*/
//const Test TestList[NB_OF_TEST]=
//{
//	  MOTOR_SPEED1_TEST,
////	  MOTOR_SPEED2_TEST,
////	  MOTOR_SPEED3_TEST,
////	  MOTOR_SPEED4_TEST,
//	  THERMO_REAR_TEST,
//	  THERMO_BAFFLE_TEST,
//	  PLENUM_RTD_TEST,
//	  STEPPER_MOTOR1_TEST,
//	  STEPPER_MOTOR2_TEST,
//	  THERMOSTAT_TEST,
//	  INTERLOCK_TEST,
//	  NB_OF_TEST
//};

//static Test* pTestState = &TestList[MOTOR_SPEED1_TEST];
static Test TestState = COMPLETED;
Test getTestState();

void TestButtonWalkthrough(Test* pteststate)
{
	static int debounceCounter=0;

	//function used to "harshly" debounce with timer and stuff for production testing
	if(GPIO_PIN_SET==HAL_GPIO_ReadPin(Button_input_GPIO_Port,Button_input_Pin))
	{
		debounceCounter++;
		if(debounceCounter > 1)
		{
			(*pteststate)++; //increment the test sequence.
			if(*pteststate >=NB_OF_TEST)
			{
				*pteststate = 0;
				Algo_setState(ZEROING_STEPPER);
			}
			debounceCounter = 0 ;
			while(GPIO_PIN_SET==HAL_GPIO_ReadPin(Button_input_GPIO_Port,Button_input_Pin)){}; // stay here if button is maintained
		}
	}
	else
	{
		debounceCounter =0;
	}
}

void TestRunner()
{
	//pTestState = &TestList[MOTOR_SPEED1_TEST];
	// we are going to use the status led to encode the test step

	switch (TestState) {
		case COMPLETED:
			//setStatusBit(TestState);
			TestButtonWalkthrough(&TestState);
			break;
		case MOTOR_SPEED1_TEST:
			vSetSpeed(1);
			//setStatusBit(TestState);
			TestButtonWalkthrough(&TestState);
			break;
		case THERMO_REAR_TEST:
			vSetSpeed(0);
			//setStatusBit(1);
			TestButtonWalkthrough(&TestState);
			break;
		case THERMO_BAFFLE_TEST:
			//setStatusBit(2);
			TestButtonWalkthrough(&TestState);
			break;
		case PLENUM_RTD_TEST:
			//setStatusBit(3);
			TestButtonWalkthrough(&TestState);
			break;
		case STEPPER_MOTOR1_TEST:
			//setStatusBit(4);
			StepperMotorProdTest(PrimaryStepper);
			TestButtonWalkthrough(&TestState);
			TestState++;
			break;
		case STEPPER_MOTOR2_TEST:
			//setStatusBit(5);
			StepperMotorProdTest(BoostStepper);
			TestButtonWalkthrough(&TestState);
			TestState++;
			break;
		case THERMOSTAT_TEST:
			//setStatusBit(6);
			TestButtonWalkthrough(&TestState);
			break;
		case INTERLOCK_TEST:
			//setStatusBit(7);
			TestButtonWalkthrough(&TestState);
			break;
		default:
			break;
	}
}
Test getTestState()
{
	return TestState;
}




