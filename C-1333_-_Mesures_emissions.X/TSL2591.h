/*
Solutions Novika.
Copyright 2021
===========================================================================

Filename :      TSL2591.h

Author(s):      Clement Soucy, CPI # 6027411

Public prefix : tsl2591

Project # : C-1333

Product: Mesure d'émissions polluantes

Creation date:  2021/06/18

Description:    TSL2591 driver.

===========================================================================

Modifications
-------------

By    |   Date     | Version | Description
------+------------+---------+---------------------------------------------
CS    | 2021/06/18 | -       | Creation
===========================================================================
*/ 
#ifndef TSL2591_H
#define	TSL2591_H

#include <xc.h>
#include <stdbool.h>


void tsl2591Initialize(void);
void tsl2591Process(void);
bool tsl2591SenseLight(uint16_t* pFull, uint16_t* pIr, float* pLux, void (*senseCompleteHandler)());
bool tsl2591IsStarted(void);
bool tsl2591IsReadyForRequest(void);
void TSLset_parameters(uint8_t gain, uint8_t integ);

uint8_t TSLgetIntegrationTime(void);


#endif	/* TSL2591_H */

