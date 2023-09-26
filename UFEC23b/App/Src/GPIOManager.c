#include "GPIOManager.h"
#include "Paramfile.h"
#include "stm32f1xx_hal.h"
#include "main.h"
#include "Algo.h"

static bool bButtonPressed = false;
static uint32_t u32PressStartTime_ms = 0;
static bool bSafetyActive = false;
static uint32_t u32SafetyStartTime_ms = 0;


void GPIOManager(Mobj *stove, uint32_t u32CurrentTime_ms)
{

	const PF_UsrParam* uParam = PB_GetUserParam();
	static uint8_t u8BlinkCounter = 0;
	static uint32_t u32ButtonBlinkStartTime_ms = 0;

	// Update Thermostat boolean based on GPIO state
	stove->bThermostatOn = (HAL_GPIO_ReadPin(Thermostat_Input_GPIO_Port,Thermostat_Input_Pin) == GPIO_PIN_RESET);
	// Update Interlock boolean based on GPIO state
	//stove->bInterlockOn = (HAL_GPIO_ReadPin(Interlock_Input_GPIO_Port,Interlock_Input_Pin) == GPIO_PIN_RESET);
	// Update Door state boolean based on GPIO state
	stove->bDoorOpen = (HAL_GPIO_ReadPin(Limit_switch_Door_GPIO_Port,Limit_switch_Door_Pin) == GPIO_PIN_SET);

	// Store button GPIO state for debounce
	//bButtonPressed = (HAL_GPIO_ReadPin(Button_Input_GPIO_Port,Button_Input_Pin) == GPIO_PIN_SET);
	bButtonPressed = (HAL_GPIO_ReadPin(Interlock_Input_GPIO_Port,Interlock_Input_Pin) == GPIO_PIN_RESET);

	if(bButtonPressed && (u32PressStartTime_ms == 0))
	{
		u32PressStartTime_ms = u32CurrentTime_ms;//Initialize timer
	}
	else if(bButtonPressed && (u32CurrentTime_ms - u32PressStartTime_ms < 100)) // Software debounce
	{
		stove->bReloadRequested = true; // Button is pressed, Update Reload Requested boolean
	}
	else if(!bButtonPressed) // Not pressed? Reset timer
	{
		u32PressStartTime_ms = 0;
	}

	if(stove->bButtonBlinkRequired)
	{
		if(u32CurrentTime_ms - u32ButtonBlinkStartTime_ms > 200)
		{
			if(u8BlinkCounter++ > 13)
			{
				stove->bButtonBlinkRequired = false;
				u8BlinkCounter = 0;
			}else
			{
				HAL_GPIO_TogglePin(GPIOA, Step3_DIR_Pin|Button_LED_Pin);
				u32ButtonBlinkStartTime_ms = u32CurrentTime_ms;//Initialize timer
			}

		}
	}

	bSafetyActive = (HAL_GPIO_ReadPin(Safety_ON_GPIO_Port,Safety_ON_Pin) == GPIO_PIN_SET);

	if(bSafetyActive && (u32SafetyStartTime_ms == 0))
	{
		u32SafetyStartTime_ms = u32CurrentTime_ms;//Initialize timer
	}
	else if(bSafetyActive && (u32CurrentTime_ms - u32SafetyStartTime_ms < 100)) // Software debounce
	{
		stove->bSafetyOn = true;
	}
	else if(!bSafetyActive) // Not active? Reset timer
	{
		u32SafetyStartTime_ms = 0;
		stove->bSafetyOn = false;
	}


}

bool GPIO_IsButtonPressed(void)
{
	return bButtonPressed;
}




