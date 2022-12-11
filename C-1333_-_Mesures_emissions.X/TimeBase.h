/*
Solutions Novika.
Copyright 2021
===========================================================================

Filename :      TimeBase.h

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
#ifndef TIMEBASE_H
#define	TIMEBASE_H

#include <xc.h>
#include <stdbool.h>

void TimeInitialize(void);

void TimeStartMsTimer(uint32_t* p_uTimer, uint32_t uMsDelay);

bool TimeIsTimerComplete(uint32_t* p_uTimer);


#endif	/* TIMEBASE_H */

