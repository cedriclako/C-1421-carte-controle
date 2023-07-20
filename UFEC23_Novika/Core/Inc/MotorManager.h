
#ifndef STEPPER_H
#define	STEPPER_H

//#include
#include <stdbool.h>
#include <stdint.h>
#include "cmsis_os.h"
#include "ParamFile.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum
{
    PrimaryStepper =  0,
    GrillStepper,
	SecondaryStepper,
    NumberOfMotors

}motor_t;

typedef enum
{
    Closing =  0,
    Opening
}motor_direction_t;


typedef struct Stepper
{
	uint8_t u8ID;
	motor_direction_t sDirection;
	uint8_t u8Position;
	uint8_t u8SetPoint;
	uint8_t u8MaxValue;
	uint8_t u8MinValue;
	uint32_t u32LastMove_ms;
	float fSecPerStep;
}StepObj;

#define STEPPER_INIT(_id, _min, _max) {.u8ID = _id, .u8MinValue = _min, .u8MaxValue = _max,.u8Position = _min, .u8SetPoint = _min}

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define RANGE(min,val,max) MAX(min,MIN(val,max))
#define UNUSED_PARAM(param)  (void)(param)

#define STEP_RANGE_PRIMARY   (PF_PRIMARY_FULL_OPEN - PF_PRIMARY_MINIMUM_OPENING)
#define STEP_RANGE_GRILL   (PF_GRILL_FULL_OPEN - PF_GRILL_CLOSED)

//#private functions
void AllMotorToZero();
void Motor_Init(void const * argument);
//public function
extern void StepperMotorProdTest(motor_t MotorId);

#ifdef	__cplusplus // Provide C++ Compatibility
}
#endif

#endif	/* STEPPER_H */
/**
 End of File
*/
