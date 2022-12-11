/*
Solutions Novika.
Copyright 2021
===========================================================================

Filename :      DS1775.h

Author(s):      Clement Soucy, CPI # 6027411

Public prefix : ds1775

Project # : C-1333

Product: Mesure d'émissions polluantes

Creation date:  2021/06/21

Description:    DS1775 temperature sensor driver.

===========================================================================

Modifications
-------------

By    |   Date     | Version | Description
------+------------+---------+---------------------------------------------
CS    | 2021/06/21 | -       | Creation
===========================================================================
*/ 
#ifndef DS1775_H
#define	DS1775_H

#include <xc.h>
#include <stdbool.h>

void ds1775Initialize(void);

bool ds1775IsDataReady(void);

void ds1775Process(void);

float ds1775GetTemperatureCelcius(void);

bool ds1775IsStarted(void);

void ds1775SetPointerToTemp(void);

void ds1775RequestRead(void);

#endif	/* DS1775_H */

