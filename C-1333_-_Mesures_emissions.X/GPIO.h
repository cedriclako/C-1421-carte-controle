/*
Solutions Novika.
Copyright 2021
===========================================================================

Filename :      GPIO.h

Author(s):      Clement Soucy, CPI # 6027411

Public prefix : Gpio

Project # : C-1333

Product: Mesure d'émissions polluantes

Creation date:  2021/06/14

Description:    Implementation of GPIO functions.

===========================================================================

Modifications
-------------

By    |   Date     | Version | Description
------+------------+---------+---------------------------------------------
CS    | 2021/06/14 | -       | Creation
===========================================================================
*/ 
#ifndef GPIO_H
#define	GPIO_H

#include <xc.h>

typedef enum
{
    eGPIO_TP_DEBUG = 0,
    eGPIO_TSL_INT,
    eGPIO_SCL,
    eGPIO_SDA,
    eGPIO_LED_ON,
    eGPIO_OVER_TEMP,
    eGPIO_CONTROL_RX,
    eGPIO_CONTROL_TX,
    eGPIO_DBG_TX,
    eGPIO_DBG_RX,


    eGPIO_NB_IO,    // Keep at the end of the list
}EGpioSignal;


void GpioInitialize(void);

void GpioWritePin(EGpioSignal eSignal, uint8_t uVal);

uint8_t GpioReadPin(EGpioSignal eSignal);

void GpioTogglePin(EGpioSignal eSignal);


#endif	/* GPIO_H */

