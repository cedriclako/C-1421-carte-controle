/*
Solutions Novika.
Copyright 2022
===========================================================================

Filename :      ChillerManager.c

Author(s):      Guillaume Caon   

Public prefix : Chiller

Project # : C-1421

Product: Mesure d'émissions polluantes

Creation date:  2023/09/16

Description:    Control fan speed according to internal module temperature

===========================================================================

Modifications
-------------

By    |   Date     | Version | Description
------+------------+---------+---------------------------------------------
GC    | 2023/09/16 | 0.1      | Creation
===========================================================================
*/ 

#include <xc.h>
#include "ChillerManager.h"
#include "mcc_generated_files/pwm1_16bit.h"
#include "ParameterFile.h"
#include "MeasureParticles.h"
#include "mcc_generated_files/pin_manager.h"

#define ChillerLowerTempLimit       30.0 //degrees celcius
#define ChillerHigherTempLimit      50.0 //degrees celcius
#define ChillerTempLimitHysteresis  2.0  //degrees celcius
#define LowSpeedDutyCycle   0x4B00  //50% duty cycle - magic number obtain with MCC generated file. Configure PWM and see PWM1_16BIT_Initialize 
#define HighSpeedDutyCycle  0x7D00  //100% duty cycle - magic number obtain with MCC generated file. Configure PWM and see PWM1_16BIT_Initialize 

void SetChillerOFF(void);
void SetChillerLow(void);
void SetChillerHigh(void);

typedef enum
{
    eBoardIsCold,
    eBoardIsWarm,
    eBoardIsHot,
    
    eChillerManage_NB_STATES,
}EChillerManageStates;


void ChillerInit(void){

    //SetChillerOFF();
}

void ChillerProcess(void){


    
    
    static bool bFirstTime = true;
    static EChillerManageStates CurrentState, LastSate = eBoardIsCold;       
    float fInternalTemp = measureGetInternalTemp();
                             
    switch (CurrentState)
    {        
        case eBoardIsCold :
        {            
            if(bFirstTime)
            {
                SetChillerOFF();
            }
            if(fInternalTemp > ChillerLowerTempLimit)
            {                
                CurrentState = eBoardIsWarm; 
            }
                       
            break;
        }
                        
        case eBoardIsWarm:
        {
            if(bFirstTime)
            {
                SetChillerLow();
            }
            if(fInternalTemp <= (ChillerLowerTempLimit - ChillerTempLimitHysteresis))
            {
                CurrentState = eBoardIsCold; 
            }
            else if (fInternalTemp > ChillerHigherTempLimit)
            {
                 CurrentState = eBoardIsHot;
            }
            break;
        }
                                        
        case eBoardIsHot :
        {                        
            if(bFirstTime)
            {
                SetChillerHigh();
            }
            if(fInternalTemp <= (ChillerHigherTempLimit - ChillerTempLimitHysteresis))
            {
                CurrentState = eBoardIsWarm; 
            }
            else if (fInternalTemp <= (ChillerLowerTempLimit - ChillerTempLimitHysteresis))
            {
                 CurrentState = eBoardIsCold;
            }
            break;
        }
        default:
        {
            break;
        }
    }
    
    bFirstTime = LastSate != CurrentState ? true : false;
    LastSate = CurrentState; 
    
}

void SetChillerOFF(void)
{
    PWM1_16BIT_Disable();
}
void SetChillerLow(void)
{   
    PWM1_16BIT_Disable();
    PWM1_16BIT_SetSlice1Output1DutyCycleRegister(LowSpeedDutyCycle); 
    PWM1_16BIT_LoadBufferRegisters();
    PWM1_16BIT_Enable();
    
}
void SetChillerHigh(void)
{
    PWM1_16BIT_Disable();
    PWM1_16BIT_SetSlice1Output1DutyCycleRegister(HighSpeedDutyCycle);  
    PWM1_16BIT_LoadBufferRegisters();
    PWM1_16BIT_Enable();
}

