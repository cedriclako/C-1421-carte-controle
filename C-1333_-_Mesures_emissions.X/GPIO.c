/*
Solutions Novika.
Copyright 2021
===========================================================================

Filename :      GPIO.c

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

#include "GPIO.h"

typedef struct
{
    volatile unsigned char* m_Port;
    volatile unsigned char* m_Lat;
    uint8_t m_uPin;
    
}SGpioObject;

static SGpioObject gs_sGpioObject[eGPIO_NB_IO];

void GpioInitialize(void)
{
    // Attribute PORT and LAT information for all IO
    gs_sGpioObject[eGPIO_TP_DEBUG].m_Lat = &LATC;
    gs_sGpioObject[eGPIO_TP_DEBUG].m_Port  =&PORTC;
    gs_sGpioObject[eGPIO_TP_DEBUG].m_uPin = 4;
    
    gs_sGpioObject[eGPIO_TSL_INT].m_Lat = &LATC;
    gs_sGpioObject[eGPIO_TSL_INT].m_Port  =&PORTC;
    gs_sGpioObject[eGPIO_TSL_INT].m_uPin = 6;
    
    gs_sGpioObject[eGPIO_SCL].m_Lat = &LATB;
    gs_sGpioObject[eGPIO_SCL].m_Port  =&PORTB;
    gs_sGpioObject[eGPIO_SCL].m_uPin = 6;
    
    gs_sGpioObject[eGPIO_SDA].m_Lat = &LATB;
    gs_sGpioObject[eGPIO_SDA].m_Port  =&PORTB;
    gs_sGpioObject[eGPIO_SDA].m_uPin = 4;
    
    gs_sGpioObject[eGPIO_LED_ON].m_Lat = &LATB;
    gs_sGpioObject[eGPIO_LED_ON].m_Port  =&PORTB;
    gs_sGpioObject[eGPIO_LED_ON].m_uPin = 5;
    
    gs_sGpioObject[eGPIO_OVER_TEMP].m_Lat = &LATA;
    gs_sGpioObject[eGPIO_OVER_TEMP].m_Port  =&PORTA;
    gs_sGpioObject[eGPIO_OVER_TEMP].m_uPin = 4;
    
    gs_sGpioObject[eGPIO_CONTROL_RX].m_Lat = &LATC;
    gs_sGpioObject[eGPIO_CONTROL_RX].m_Port  =&PORTC;
    gs_sGpioObject[eGPIO_CONTROL_RX].m_uPin = 2;
    
    gs_sGpioObject[eGPIO_CONTROL_TX].m_Lat = &LATC;
    gs_sGpioObject[eGPIO_CONTROL_TX].m_Port  =&PORTC;
    gs_sGpioObject[eGPIO_CONTROL_TX].m_uPin = 1;
    
    gs_sGpioObject[eGPIO_DBG_TX].m_Lat = &LATC;
    gs_sGpioObject[eGPIO_DBG_TX].m_Port  =&PORTC;
    gs_sGpioObject[eGPIO_DBG_TX].m_uPin = 0;
    
    gs_sGpioObject[eGPIO_DBG_RX].m_Lat = &LATA;
    gs_sGpioObject[eGPIO_DBG_RX].m_Port  =&PORTA;
    gs_sGpioObject[eGPIO_DBG_RX].m_uPin = 2;
    
}

void GpioWritePin(EGpioSignal eSignal, uint8_t uVal)
{
    if (uVal == 1)
    {
        *gs_sGpioObject[eSignal].m_Lat |= 1 << gs_sGpioObject[eSignal].m_uPin;
    }
    else if (uVal == 0)
    {
        *gs_sGpioObject[eSignal].m_Lat &= 0xFF ^ (1 << gs_sGpioObject[eSignal].m_uPin);
    }
}

uint8_t GpioReadPin(EGpioSignal eSignal)
{
    return (*gs_sGpioObject[eSignal].m_Port & (1 << gs_sGpioObject[eSignal].m_uPin)) >> gs_sGpioObject[eSignal].m_uPin;
}

void GpioTogglePin(EGpioSignal eSignal)
{
    *gs_sGpioObject[eSignal].m_Lat ^= 1 << gs_sGpioObject[eSignal].m_uPin;
}
