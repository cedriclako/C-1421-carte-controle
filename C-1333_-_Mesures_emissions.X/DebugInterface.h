/*
Solutions Novika.
Copyright 2021
===========================================================================

Filename :      DebugInterface.h

Author(s):      Clement Soucy, CPI # 6027411

Public prefix : Debug

Project # : C-1333

Product: Mesure d'émissions polluantes

Creation date:  2021/06/21

Description:    Implementation debug console.

===========================================================================

Modifications
-------------

By    |   Date     | Version | Description
------+------------+---------+---------------------------------------------
CS    | 2021/06/21 | -       | Creation
===========================================================================
*/ 
#ifndef DEBUGINTERFACE_H
#define	DEBUGINTERFACE_H

#include <xc.h>

void DebugInitialize(void);
void DebugProcess(void);

void putch(char txData);
char getch(void);


#endif	/* DEBUGINTERFACE_H */

