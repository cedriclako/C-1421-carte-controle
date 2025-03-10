/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : Hmi.c
  * @brief          : Human Machine Interface (Buttons LEDs)
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
#include "ParticlesManager.h"
#include "algo.h"
#include "main.h"
#include "ProdTest.h"
#include "Hmi.h"

/* Private includes ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define SetButtonLed_ON() HAL_GPIO_WritePin(Button_LED_GPIO_Port,Button_LED_Pin,SET)
#define SetButtonLed_OFF() HAL_GPIO_WritePin(Button_LED_GPIO_Port,Button_LED_Pin,RESET)

//#define Set_STATUS_BIT0_ON() HAL_GPIO_WritePin(STATUS_LED0_GPIO_Port,STATUS_LED0_Pin,RESET)
#define Set_STATUS_BIT1_ON() HAL_GPIO_WritePin(STATUS_LED1_GPIO_Port,STATUS_LED1_Pin,RESET)
#define Set_STATUS_BIT2_ON() HAL_GPIO_WritePin(STATUS_LED2_GPIO_Port,STATUS_LED2_Pin,RESET)

//#define Set_STATUS_BIT0_OFF() HAL_GPIO_WritePin(STATUS_LED0_GPIO_Port,STATUS_LED0_Pin,SET)
#define Set_STATUS_BIT1_OFF() HAL_GPIO_WritePin(STATUS_LED1_GPIO_Port,STATUS_LED1_Pin,SET)
#define Set_STATUS_BIT2_OFF() HAL_GPIO_WritePin(STATUS_LED2_GPIO_Port,STATUS_LED2_Pin,SET)

#define ToggleButtonLed() HAL_GPIO_TogglePin(Button_LED_GPIO_Port, Button_LED_Pin)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function ----------------------------------------------------------*/

/* Model bits moved to ESP32 /////////////////////////////////////////////////////////////////////
inline int Read_RevBit()
{
	int RevisionNumber;
	RevisionNumber = (int)HAL_GPIO_ReadPin(Rev_bit0_GPIO_Port,Rev_bit0_Pin) + HAL_GPIO_ReadPin(Rev_bit1_GPIO_Port,Rev_bit1_Pin)*2;
	return RevisionNumber;

}
*/

FurnaceModel readModel()
{
	FurnaceModel model = 0;  //default to Heatmax
/*
	if(GPIO_PIN_SET == HAL_GPIO_ReadPin(Model_bit2_GPIO_Port,Model_bit2_Pin))
	{
		model = 1;
	}
	if(GPIO_PIN_SET == HAL_GPIO_ReadPin(Model_bit1_GPIO_Port,Model_bit1_Pin))
	{
		model +=2 ;
	}
	if(GPIO_PIN_SET == HAL_GPIO_ReadPin(Model_bit0_GPIO_Port,Model_bit0_Pin))
	{
		model +=4;
	}
*/
	return model;
}

void HmiManager()
{
	static uint32_t LastButtonPressedTime_ms = 0;
	static uint32_t LastButttonToggle_ms =0;
	uint32_t LastButttonToggleTemp_ms =0;
	static uint32_t thermocoupleTestPeriod =0;
	static Test currentState;
	static bool ButtonBlinkingrequired = false;
	static uint8_t buttonblinkrequirecount = 0;
    bool tStatDemand;
    bool interlockActive;

	for(;;)
	{

		osDelay(50);
		State algostate = Algo_getState();
		uint32_t kerneltime = osKernelSysTick();

		tStatDemand = (HAL_GPIO_ReadPin(Thermostat_Input_GPIO_Port,Thermostat_Input_Pin) == GPIO_PIN_RESET);
		interlockActive = (HAL_GPIO_ReadPin(Interlock_Input_GPIO_Port,Interlock_Input_Pin) == GPIO_PIN_RESET);

		Algo_setThermostatRequest(tStatDemand);
		Algo_setInterlockRequest(interlockActive);

		if(algostate !=PRODUCTION_TEST)
		{
			//if(GPIO_PIN_SET==HAL_GPIO_ReadPin(Button_input_GPIO_Port,Button_input_Pin))
			//{
			//	HAL_GPIO_TogglePin(Button_LED_GPIO_Port, Button_LED_Pin);
			//}
			//else
			//{
				if( (tStatDemand || Algo_getInterlockRequest()) && (Algo_getState() !=SAFETY && Algo_getState() != OVERTEMP && !ButtonBlinkingrequired) )
				{
					if(Algo_getInterlockRequest() || Algo_getFrontTemp() < 4000) //if rear temp below 400F, furnace is too cold and reignition is needded
					{
						SetButtonLed_OFF();
					}
					else
					{
						SetButtonLed_ON();
					}
				}
				else if(Algo_getBaffleTemp()>4000 || Algo_getState() ==SAFETY || Algo_getState() ==OVERTEMP || ButtonBlinkingrequired)
				{
					if(Algo_getState() ==SAFETY || Algo_getState() ==OVERTEMP)
					{
						LastButttonToggleTemp_ms = LastButttonToggle_ms+100;
					}
					else if (ButtonBlinkingrequired)
					{
						LastButttonToggleTemp_ms = LastButttonToggle_ms+50;
					}
					else
					{
						LastButttonToggleTemp_ms = LastButttonToggle_ms+1500;
					}
					if((LastButttonToggleTemp_ms) < kerneltime) //1Hz
					{
						if(ButtonBlinkingrequired && (buttonblinkrequirecount >=0))
						{
							buttonblinkrequirecount--;
							if(buttonblinkrequirecount ==0)
							{
								ButtonBlinkingrequired = false;
							}
						}
						LastButttonToggle_ms = kerneltime;
						HAL_GPIO_TogglePin(Button_LED_GPIO_Port, Button_LED_Pin);
					}
				}
				else
				{
					SetButtonLed_OFF();
				}
			//}
		}
		else
		{
			currentState = getTestState();
			void Algo_clearReloadRequest(); // in case we generate an event on function entry
			if(currentState == THERMO_REAR_TEST)
				thermocoupleTestPeriod = (float)1000/Algo_getFrontTemp()*800;
			else if (currentState== THERMO_BAFFLE_TEST)
				thermocoupleTestPeriod = (float)1000/Algo_getBaffleTemp()*800;
			else if (currentState== PLENUM_RTD_TEST)
				thermocoupleTestPeriod = (float)1000/Algo_getPlenumTemp()*1000;

			if (currentState == THERMO_REAR_TEST
					|| currentState == THERMO_BAFFLE_TEST
					|| currentState == PLENUM_RTD_TEST)
			{
				if((LastButttonToggle_ms+thermocoupleTestPeriod) < kerneltime)
				{
					LastButttonToggle_ms = kerneltime;
					ToggleButtonLed();
				}
			}
			else if (currentState == THERMOSTAT_TEST)
			{
				Algo_getThermostatRequest()?SetButtonLed_ON():SetButtonLed_OFF();
			}
			else if(currentState == INTERLOCK_TEST)
			{
				Algo_getInterlockRequest()?SetButtonLed_ON():SetButtonLed_OFF();
			}
			else
			{
				SetButtonLed_OFF();
			}
		}
		if(algostate !=PRODUCTION_TEST)
		{
			if(GPIO_PIN_SET==HAL_GPIO_ReadPin(Button_Input_GPIO_Port,Button_Input_Pin))
			{
				if ((LastButtonPressedTime_ms+100) < kerneltime)
				{
					Algo_startChargement(kerneltime);
					ButtonBlinkingrequired = true;
					buttonblinkrequirecount = 6;
					//Particle_setConfig();
				}
				else
				{
					LastButtonPressedTime_ms = osKernelSysTick();
				}
			}
		}

//Status bit Handling
		switch(algostate)
		{
			case SAFETY:
			case OVERTEMP:
				Set_STATUS_BIT2_ON();
				break;
			default:
				Set_STATUS_BIT2_OFF();
				break;
		}
		//tStatDemand?Set_STATUS_BIT0_ON():Set_STATUS_BIT0_OFF();
		interlockActive?Set_STATUS_BIT1_ON():Set_STATUS_BIT1_OFF();
	}
}

void setStatusBit(uint8_t status)
{

	//if(status & 4)
	//{
		//Set_STATUS_BIT0_ON();
	//}
	//else
	//{
		//Set_STATUS_BIT0_OFF();
	//}
	if(status & 2)
	{
		Set_STATUS_BIT1_ON();
	}
	else
	{
		Set_STATUS_BIT1_OFF();
	}
	if(status & 1)
	{
		Set_STATUS_BIT2_ON();
	}
	else
	{
		Set_STATUS_BIT2_OFF();
	}
}
