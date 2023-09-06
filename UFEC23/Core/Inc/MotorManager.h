
#ifndef STEPPER_H
#define	STEPPER_H

//#include
#include <stdbool.h>
#include <stdint.h>
#include "cmsis_os.h"
#include "air_input.h"
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

typedef struct _MotorControlStruct
{
    int8_t i8PositionRequested;
    int8_t i8CurrentPosition;
    uint16_t ui16CurrentPositionInStep;
    bool bClosing;

}MotorControl_t;

typedef enum
{
    Closing =  0,
    Opening
}motor_direction_t;

typedef enum {
  MOT_PLENUM_STOP = 0,
  MOT_PLENUM_LOW,
  MOT_PLENUM_MID_LOW,
  MOT_PLENUM_MID_HIGH,
  MOT_PLENUM_HIGH,
} Mot_FanSpeed;

//all temperature in tenth of degrees F
typedef struct FanSpeedKipKopParam{
	int16_t KipSpeed1;
	int16_t KopSpeed1;
	int16_t KipSpeed2;
	int16_t KopSpeed2;
	int16_t KipSpeed3;
	int16_t KopSpeed3;
	int16_t KipSpeed4;
	int16_t KopSpeed4;
	int16_t NoDemandKipSpeed1;
	int16_t NoDemandKopSpeed1;
	int16_t NoDemandKipSpeed2;
	int16_t NoDemandKopSpeed2;
	int16_t NoDemandKipSpeed3;
	int16_t NoDemandKopSpeed3;
	int16_t NoDemandKipSpeed4;
	int16_t NoDemandKopSpeed4;
}FanSpeedKipKopParam_t;

//static inline LONG PROPORTION( LONG percent, LONG value)
//{
//    percent = RANGE( 0, percent, 100);
//    return (percent * value) / 100;
//}
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define RANGE(min,val,max) MAX(min,MIN(val,max))
#define UNUSED_PARAM(param)  (void)(param)

#define STEP_RANGE_PRIMARY   (PF_PRIMARY_FULL_OPEN - PF_PRIMARY_MINIMUM_OPENING)
#define STEP_RANGE_GRILL   (PF_GRILL_FULL_OPEN - PF_GRILL_CLOSED)

//#private functions
void vFullOpen(void);
void vOpen(uint8_t Percent);
void vClose(uint8_t Percent);
void vStop (void);
void StepperPosPercent (uint8_t PosPercentDesire);
void GoTo0(void);
void mot_SetStepMotor(motor_t MotorId, uint8_t ui8Opening);
void MotorManager(void);
void Steppermanager(void const * argument);
void AllMotorToZero();
MotorControl_t* pstGetMotor(motor_t Index);
void MotorManagerTask(void const * argument);
void vSetSpeed(Mot_FanSpeed RequestedSpeed);
bool Mot_InPosition( AirInput * self,motor_t Motorid);
//public function
extern Mot_FanSpeed Mot_getFanSpeed();
extern void StepperMotorProdTest(motor_t MotorId);
void manageFans(int baffleTemp, const PF_UsrParam* Param);

#ifdef	__cplusplus // Provide C++ Compatibility
}
#endif

#endif	/* STEPPER_H */
/**
 End of File
*/
