
#ifndef STEPPER_H
#define	STEPPER_H

//#include
#include <stdbool.h>
#include <stdint.h>
#include "cmsis_os.h"
#include "stm32f1xx_hal.h"
#include "ParamFile.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define NOVIKA_SETUP (1)

typedef enum
{
    PrimaryStepper =  0,
    GrillStepper,
	SecondaryStepper,
    NumberOfMotors

}motor_t;

typedef enum
{
#if NOVIKA_SETUP
	Opening =  0,
    Closing
#else
	Closing =  0,
    Opening
#endif

}motor_direction_t;

typedef struct Pin_set
{
	uint16_t PWM_PIN;
	uint16_t ENABLE_PIN;
	uint16_t RESET_PIN;
	uint16_t LOW_CURRENT_PIN;
	uint16_t DIRECTION_PIN;
	uint16_t SWITCH_PIN;

	GPIO_TypeDef* PWM_PORT;
	GPIO_TypeDef* ENABLE_PORT;
	GPIO_TypeDef* RESET_PORT;
	GPIO_TypeDef* LOW_CURRENT_PORT;
	GPIO_TypeDef* DIRECTION_PORT;
	GPIO_TypeDef* SWITCH_PORT;
}Pin_set_t;


typedef struct Stepper
{
	motor_direction_t sDirection;
	Pin_set_t sPins;
	uint8_t u8Position;
	uint8_t u8SetPoint;
	uint8_t u8MaxValue;
	uint8_t u8MinValue;
	uint32_t u32LastMove_ms;
	float fSecPerStep;
}StepObj;

#define STEPPER_INIT(_min, _max, _PP, _EP, _RP, _LP, _DP, _SP,_PG, _EG, _RG, _LG, _DG, _SG) {.u8MinValue = _min, .u8MaxValue = _max,.u8Position = 100, .u8SetPoint = 100,.sPins = {.PWM_PIN = _PP,.ENABLE_PIN = _EP,.RESET_PIN = _RP,.LOW_CURRENT_PIN = _LP,.DIRECTION_PIN = _DP,.SWITCH_PIN = _SP, .PWM_PORT = _PG,.ENABLE_PORT = _EG,.RESET_PORT =  _RG,.LOW_CURRENT_PORT = _LG,.DIRECTION_PORT = _DG, .SWITCH_PORT = _SG}}

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define RANGE(min,val,max) MAX(min,MIN(val,max))
#define UNUSED_PARAM(param)  (void)(param)


void Motor_task(void const * argument);


#ifdef	__cplusplus // Provide C++ Compatibility
}
#endif

#endif	/* STEPPER_H */
/**
 End of File
*/
