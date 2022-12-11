/*
Solutions Novika.
Copyright 2021
===========================================================================

Filename :      TimeBase.c

Author(s):      Clement Soucy, CPI # 6027411

Public prefix : Time

Project # : C-1333

Product: Mesure d'émissions polluantes

Creation date:  2021/06/09

Description:    Implementation of a time base using timer peripheral.

===========================================================================

Modifications
-------------

By    |   Date     | Version | Description
------+------------+---------+---------------------------------------------
CS    | 2021/06/09 | -       | Creation
===========================================================================
*/

#include "TimeBase.h"

#include "mcc_generated_files/tmr0.h"

uint32_t uTimerCountMs;

void TimerInterruptHandler(void);

void TimeInitialize(void)
{
    uTimerCountMs = 0;
    
    
    
    TMR0_SetInterruptHandler(TimerInterruptHandler);
    
    TMR0_StartTimer();
    
}

void TimeStartMsTimer(uint32_t* p_uTimer, uint32_t uMsDelay)
{
    *p_uTimer = uTimerCountMs + uMsDelay;
}

bool TimeIsTimerComplete(uint32_t* p_uTimer)
{
    return (((int32_t)*p_uTimer - (int32_t)uTimerCountMs) <= 0);
}

void TimerInterruptHandler(void)
{
    uTimerCountMs ++;
        
    TMR0H = 0x1E;

}
