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
typedef enum
{
	FAN_AFK,
	FAN_FAN_L,

	FAN_NUM_OF_FANS
}Fan_t;

typedef struct Fan_pin_set
{
	uint16_t MODULATION_PIN;

	GPIO_TypeDef* MODULATION_PORT;

}Fan_pin_set_t;

typedef struct Fan
{

	Fan_pin_set_t sPins;

	uint16_t u16FanSpeedPercent;
	uint16_t u16MinSpeedPercent;
	uint16_t u16MaxSpeedPercent;

	bool bEnabled;



}FanObj;


#define FAN_INIT(_min, _max, _PM, _GM) {.u16MinSpeedPercent = _min, .u16MaxSpeedPercent = _max,.u16FanSpeedPercent = 0, .bEnabled = false, .sPins = {.MODULATION_PIN = _PM,.MODULATION_PORT = _GM}}

void Fan_RaiseZeroFlag(void);

void Fan_Process(Mobj *stove, uint32_t u32CurrentTime_ms);

void Fan_Init(void);

void Fan_EnableZeroDetect(void);

#endif /* INC_FANMANAGER_H_ */
