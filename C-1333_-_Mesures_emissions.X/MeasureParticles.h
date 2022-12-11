/*
Solutions Novika.
Copyright 2021
===========================================================================

Filename :      MeasureParticles.h

Author(s):      Clement Soucy, CPI # 6027411

Public prefix : measureParticles

Project # : C-1333

Product: Mesure d'émissions polluantes

Creation date:  2021/06/22

Description:    Higher level code used to measure particle emissions by 
                controlling the LED state and read data from embedded sensors.

===========================================================================

Modifications
-------------

By    |   Date     | Version | Description
------+------------+---------+---------------------------------------------
CS    | 2021/06/22 | -       | Creation
===========================================================================
*/ 
#ifndef MEASUREPARTICLES_H
#define	MEASUREPARTICLES_H

#include <xc.h>
#include <stdbool.h>

typedef enum
{
    eSUB_STATE_DARK = 0,
    eSUB_STATE_LIGHTED,
    eSUB_STATE_ZERO,

    eSUB_STATE_NB_STATES
}EMeasureParticlesSubState;

typedef enum
{
    eMEASURE_PARTICLES_NOT_STARTED = 0,
    eMEASURE_PARTICLES_REQUEST_NOT_SENT,
    eMEASURE_PARTICLES_REQUEST_SENT,
    eMEASURE_PARTICLES_DATA_RECEIVED,
    eMEASURE_PARTICLES_SET_ZERO,
    eMEASURE_PARTICLES_UPDATE_BRIDGE,
    eMEASURE_PARTICLES_RECONFIGURE,

    eMEASURE_PARTICLES_NB_STATES,
}EMeasureParticlesStates;


typedef struct
{
    EMeasureParticlesStates m_eState;
    EMeasureParticlesSubState m_eSubState;
    
    float m_fTemperatureCelcius;
    float m_fInitial_temperature;
    
    uint16_t m_uFullDark;
    uint16_t m_uIrDark;
    float m_fLuxDark;
    
    uint16_t m_uFullLighted;
    uint16_t m_uIrLighted;
    float m_fLuxLighted;
    
    float m_fLuxZero;
    uint32_t m_uMillisCounter;
    uint32_t m_uLastRead;
    uint16_t m_uMeasureInterval;
    
    uint8_t integrationTime;
    
    uint8_t dacValue;
    uint8_t dacArray[4];
    uint16_t adcValue;
    float currentCmd;
    
}SMeasureParticlesObject;

void measureParticlesInitialize(void);

void measureParticlesProcess(void);

void measureParticlesTogglePrintEnable(void);

void measureParticlesToggleAcqEnable(void);

void measureParticlesSetZero(void);

uint8_t measureParticlesGetDacValue(void);

void measureParticlesSetDacValue(uint8_t value);

void measureParticlesRequestReconfigure(void);

bool measureParticlesReadyForConfig(void);

void measureParticlesRequestReconfigure(void);

void measureParticlesSetParameters(uint8_t DAC, uint16_t time_window);

//void Measure_Set_Current(void);


#endif	/* MEASUREPARTICLES_H */

