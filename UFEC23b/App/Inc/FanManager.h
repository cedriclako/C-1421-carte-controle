/*
 * FanManager.h
 *
 *  Created on: 26 sept. 2023
 *      Author: crichard
 */

#ifndef INC_FANMANAGER_H_
#define INC_FANMANAGER_H_

#include <stdbool.h>
#include <stdint.h>
#include "cmsis_os.h"
#include "stm32f1xx_hal.h"
#include "ParamFile.h"

extern int fan1speed;
extern int fan2speed;

typedef enum
{
	FAN_DISTRIB,
	FAN_BLOWER,

	FAN_NUM_OF_FANS
}Fan_t;

typedef enum
{
	FSPEED_OFF,
	FSPEED_LOW,
	FSPEED_HIGH,
	FSPEED_AUTO,

	FSPEED_NUM_OF_SPEEDS
}Fan_Speed_t;

typedef struct Fan_pin_set
{
	uint16_t MODULATION_PIN;

	GPIO_TypeDef* MODULATION_PORT;

}Fan_pin_set_t;

typedef struct Fan
{

	Fan_pin_set_t sPins;
	Fan_Speed_t eSpeed;
	const char* szSpeedKey;

	TIM_HandleTypeDef* sStartTimer;
	TIM_HandleTypeDef* sStopTimer;

	uint16_t u16SpeedPercent;

	const char* szLowSpeedKey;

	bool bEnabled;
	Fan_Speed_t ePreviousSpeed;

}FanObj;


#define FAN_INIT(_key, _low, _uTimer, _dTimer, _PM, _GM) {.szLowSpeedKey = _low, .szSpeedKey = _key, .sStartTimer = _uTimer, .sStopTimer = _dTimer, .bEnabled = false, .u16SpeedPercent = 0, .eSpeed = FSPEED_OFF, .ePreviousSpeed = FSPEED_OFF,  .sPins = {.MODULATION_PIN = _PM,.MODULATION_PORT = _GM}}

void Fan_Process(Mobj *stove);

void Fan_Init(void);

void Fan_SetToManual(void);
void Fan_SetOutOfManual(void);
void Fan_StartPulseSPEED3(void);
void Fan_StopPulseSPEED3(void);
void Fan_StartPulseSPEED1(void);
void Fan_StopPulseSPEED1(void);

Fan_Speed_t Fan_GetSpeed(Fan_t FanID);
const char* Fan_GetSpeedString(Fan_t FanID);



#endif /* INC_FANMANAGER_H_ */
