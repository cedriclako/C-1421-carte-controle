#include "GPIOManager.h"
#include "stm32f1xx_hal.h"
#include "main.h"
#include "Algo.h"

static bool bButtonPressed = false;
static uint32_t u32PressStartTime_ms = 0;

void GPIOManager(Mobj *stove, uint32_t u32CurrentTime_ms)
{

	stove->bThermostatOn = (HAL_GPIO_ReadPin(Thermostat_Input_GPIO_Port,Thermostat_Input_Pin) == GPIO_PIN_RESET);
	stove->bInterlockOn = (HAL_GPIO_ReadPin(Interlock_Input_GPIO_Port,Interlock_Input_Pin) == GPIO_PIN_RESET);

	bButtonPressed = (HAL_GPIO_ReadPin(Button_Input_GPIO_Port,Button_Input_Pin) == GPIO_PIN_SET);//TODO: add some sort of debounce

	if(bButtonPressed && (u32PressStartTime_ms == 0))
	{
		u32PressStartTime_ms = u32CurrentTime_ms;
	}
	else if(bButtonPressed && (u32CurrentTime_ms - u32PressStartTime_ms < 100))
	{
		stove->bReloadRequested = true;
	}
	else if(!bButtonPressed)
	{
		u32PressStartTime_ms = 0;
	}


}

bool GPIO_IsButtonPressed(void)
{
	return bButtonPressed;
}

