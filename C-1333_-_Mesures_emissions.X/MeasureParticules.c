/*
Solutions Novika.
Copyright 2021
===========================================================================

Filename :      MeasureParticles.c

Author(s):      Clement Soucy, CPI # 6027411
 *              Charles Richard, CPI # 6045522

Public prefix : measureParticles

Project # : C-1333, C-1421

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
CR    | 2022/10/14 | 0.1     | Functional prototype (incorporates many changes)
===========================================================================
*/ 
#include <stdio.h>
#include "TSL2591.h"
#include "DS1775.h"
#include "GPIO.h"
#include "mcc_generated_files/tmr0.h"
#include "mcc_generated_files/dac1.h"
#include "mcc_generated_files/adcc.h"
#include "mcc_generated_files/memory.h"
#include "ControlBridge.h"
#include "MeasureParticles.h"


#define LED_ON 1
#define LED_OFF 0

#define EE_FIRST_CONF_ADDR 0x00
#define EE_DAC_ADDR 0x10
#define EE_CH0_ADDR 0x20
#define EE_CH1_ADDR 0x30
#define EE_FIRE_COUNTER_ADDR 0x40

static SMeasureParticlesObject gs_sMeasPartObject;
static bool gs_bPrintEnabled;
static bool gs_bacqEnabled;
static bool gs_bEvalCurrent;
static bool gs_bReconfigure;
static uint8_t gs_uCurrentIndex;

void measureParticlesSenseCompleteCallback(void);
void measureParticlesPrintData(void);
void MeasureTimerInterrupt(void);
void measureSetLED(float currentSetting);

void measureParticlesInitialize(void)
{
    gs_sMeasPartObject.m_eState = eMEASURE_PARTICLES_NOT_STARTED;
    gs_sMeasPartObject.m_eSubState = eSUB_STATE_DARK;
    
    gs_sMeasPartObject.m_fTemperatureCelcius = 0;
    gs_sMeasPartObject.m_fInitial_temperature = 0;
    
    gs_sMeasPartObject.m_uFullDark = 0;
    gs_sMeasPartObject.m_uIrDark = 0;
    gs_sMeasPartObject.m_fLuxDark = 0;
    
    gs_sMeasPartObject.m_uFullLighted = 0;
    gs_sMeasPartObject.m_uIrLighted = 0;
    gs_sMeasPartObject.m_fLuxLighted = 0;
    
    gs_sMeasPartObject.m_fLuxZero = 0;
    gs_sMeasPartObject.integrationTime = TSLgetIntegrationTime()+1;
    gs_sMeasPartObject.m_uLastRead = 0;
    gs_sMeasPartObject.m_uMeasureInterval = 2000;

    gs_sMeasPartObject.dacValue = 130;
    gs_sMeasPartObject.dacArray[0] = 130;
    gs_sMeasPartObject.dacArray[1] = 134;
    gs_sMeasPartObject.dacArray[2] = 136;
    gs_sMeasPartObject.dacArray[3] = 137;
    gs_bEvalCurrent = false;
    gs_uCurrentIndex = 3;
    
    DAC1_SetOutput(gs_sMeasPartObject.dacValue);
    

    gs_sMeasPartObject.adcValue = 0;
    gs_sMeasPartObject.currentCmd = 5.2; //mA
    //measureSetLED(gs_sMeasPartObject.currentCmd);
    
    TMR0_SetInterruptHandler(MeasureTimerInterrupt);
    TMR0_StartTimer();
    
    gs_bPrintEnabled = true;
    gs_bacqEnabled = true;
    gs_bReconfigure = false;
}

void measureParticlesProcess(void)
{
    switch (gs_sMeasPartObject.m_eState)
    {
        case eMEASURE_PARTICLES_NOT_STARTED:
        {
            if (tsl2591IsStarted() && ds1775IsStarted()) // give time to drivers to configure the ICs at startup
            {
                gs_sMeasPartObject.m_eState = eMEASURE_PARTICLES_REQUEST_NOT_SENT;
            }
            break;
        }
        case eMEASURE_PARTICLES_REQUEST_NOT_SENT:
        {
            if(gs_bReconfigure)
            {
                gs_sMeasPartObject.m_eState = eMEASURE_PARTICLES_RECONFIGURE;
                break;
            }
            if(gs_bacqEnabled)
            {
                if (gs_sMeasPartObject.m_eSubState == eSUB_STATE_DARK && (gs_sMeasPartObject.m_uMillisCounter - gs_sMeasPartObject.m_uLastRead) > (gs_sMeasPartObject.m_uMeasureInterval))
                {
                    ds1775RequestRead();                
                    // Request light sensing without activating LED
                    // Check if driver is ready
                    if (tsl2591IsReadyForRequest())
                    {
                        tsl2591SenseLight(&gs_sMeasPartObject.m_uFullDark, &gs_sMeasPartObject.m_uIrDark, &gs_sMeasPartObject.m_fLuxDark, measureParticlesSenseCompleteCallback);
                        gs_sMeasPartObject.m_eState = eMEASURE_PARTICLES_REQUEST_SENT;
                    }
                }
                else if (gs_sMeasPartObject.m_eSubState == eSUB_STATE_LIGHTED)
                {
                    // Activate LED and request light sensing
                    // Check if driver is ready
                    if (tsl2591IsReadyForRequest())
                    {
                        //GpioWritePin(eGPIO_LED_ON, LED_ON);
                        tsl2591SenseLight(&gs_sMeasPartObject.m_uFullLighted, &gs_sMeasPartObject.m_uIrLighted, &gs_sMeasPartObject.m_fLuxLighted, measureParticlesSenseCompleteCallback);
                        gs_sMeasPartObject.m_eState = eMEASURE_PARTICLES_REQUEST_SENT;
                    }
                }
            }
            break;
        }
        case eMEASURE_PARTICLES_REQUEST_SENT:
        {
            // Waiting for sense complete callback
            break;
        }
        case eMEASURE_PARTICLES_DATA_RECEIVED:
        {
            gs_sMeasPartObject.m_fTemperatureCelcius = ds1775GetTemperatureCelcius();
            if(gs_sMeasPartObject.m_fInitial_temperature == 0)
            {
                gs_sMeasPartObject.m_fInitial_temperature = gs_sMeasPartObject.m_fTemperatureCelcius;
            }

            if (gs_sMeasPartObject.m_eSubState == eSUB_STATE_DARK)
            {
                // activate LED
                //GpioWritePin(eGPIO_LED_ON, LED_ON);
                //Measure_Set_Current();
                
            if(gs_bEvalCurrent)
            {
                if(gs_uCurrentIndex++ >= 3)
                {
                    gs_uCurrentIndex = 0;
                }
                DAC1_SetOutput(gs_sMeasPartObject.dacArray[gs_uCurrentIndex]);
            }
                
                DAC1_Enable();
                
                // Change state and sub state
                gs_sMeasPartObject.m_eSubState = eSUB_STATE_LIGHTED;
                gs_sMeasPartObject.m_eState = eMEASURE_PARTICLES_REQUEST_NOT_SENT;
            }
            else if (gs_sMeasPartObject.m_eSubState == eSUB_STATE_LIGHTED)
            {
                // De-activate LED
                //GpioWritePin(eGPIO_LED_ON, LED_OFF);
                gs_sMeasPartObject.adcValue = (uint16_t) ADCC_GetSingleConversion(channel_ANB7);
                DAC1_Disable();
                
                // Change state and sub state
                gs_sMeasPartObject.m_eSubState = eSUB_STATE_DARK;
                gs_sMeasPartObject.m_eState = eMEASURE_PARTICLES_UPDATE_BRIDGE;
                bridgeDataRDY();
                // print data in console if it is enabled
                measureParticlesPrintData();
            }
            else if (gs_sMeasPartObject.m_eSubState == eSUB_STATE_ZERO)
            {
                // De-activate LED
                //GpioWritePin(eGPIO_LED_ON, LED_OFF);
                DAC1_Disable();
                gs_sMeasPartObject.m_fLuxZero = gs_sMeasPartObject.m_fLuxLighted;
                
                if (gs_bPrintEnabled)
                {
                    printf("ZERO SET TO: %.3f lux \r\n",gs_sMeasPartObject.m_fLuxZero);
                }
                // Change state and sub state
                gs_sMeasPartObject.m_eSubState = eSUB_STATE_DARK;
                gs_sMeasPartObject.m_eState = eMEASURE_PARTICLES_REQUEST_NOT_SENT;
            }
            break;
        }
        case eMEASURE_PARTICLES_SET_ZERO:
        {
            // activate LED
            //GpioWritePin(eGPIO_LED_ON, LED_ON);
            DAC1_Enable();
                
            // Change state and sub state
            gs_sMeasPartObject.m_eSubState = eSUB_STATE_ZERO;

            if (tsl2591IsReadyForRequest())
            {
                tsl2591SenseLight(&gs_sMeasPartObject.m_uFullLighted, &gs_sMeasPartObject.m_uIrLighted, &gs_sMeasPartObject.m_fLuxLighted, measureParticlesSenseCompleteCallback);
                gs_sMeasPartObject.m_eState = eMEASURE_PARTICLES_REQUEST_SENT;
            }
                
            break;
        }
        case eMEASURE_PARTICLES_UPDATE_BRIDGE:
            if(isBridgeRDY())
            {
                controlBridge_update(&gs_sMeasPartObject);
                gs_sMeasPartObject.m_eState = eMEASURE_PARTICLES_REQUEST_NOT_SENT;
            }
            break;
        case eMEASURE_PARTICLES_RECONFIGURE:
            if(!gs_bReconfigure && tsl2591IsStarted())
            {
                gs_sMeasPartObject.integrationTime = TSLgetIntegrationTime()+1;
                DAC1_SetOutput(gs_sMeasPartObject.dacValue);
                gs_sMeasPartObject.m_eState = eMEASURE_PARTICLES_REQUEST_NOT_SENT;
            }

            break;
        default:
        {
            break;
        }
    }
}

void measureParticlesSenseCompleteCallback(void)
{
    gs_sMeasPartObject.m_eState = eMEASURE_PARTICLES_DATA_RECEIVED;
    
}

bool measureParticlesReadyForConfig(void)
{
    return gs_sMeasPartObject.m_eState == eMEASURE_PARTICLES_RECONFIGURE;
}

void measureParticlesRequestReconfigure(void)
{
    gs_bReconfigure = true;    
}

void measureParticlesSetParameters(uint8_t DAC, uint16_t time_window)
{
    gs_sMeasPartObject.dacValue = DAC;
    gs_sMeasPartObject.m_uMeasureInterval = time_window;
    gs_bReconfigure = false;
}
void measureParticlesTogglePrintEnable(void)
{
    gs_bPrintEnabled = !gs_bPrintEnabled;
}

void measureParticlesToggleAcqEnable(void)
{
    gs_bacqEnabled = !gs_bacqEnabled;
    
    if(gs_bacqEnabled)
    {
        printf("Starting acquisition\r\n");
        DAC1_Enable();
    }
    else
    {
        printf("Acquisition paused\r\n");
        DAC1_Disable();
    }
}

void measureParticlesSetZero(void)
{
    if(gs_sMeasPartObject.m_eState != eMEASURE_PARTICLES_NOT_STARTED)
    {
        gs_sMeasPartObject.m_eState = eMEASURE_PARTICLES_SET_ZERO;  
    }  
}

void measureParticlesPrintData(void)
{
    float lux_net;
    float timeInSeconds;
    
    gs_sMeasPartObject.m_uLastRead = gs_sMeasPartObject.m_uMillisCounter;
    
    if (gs_bPrintEnabled)
    {
        lux_net = gs_sMeasPartObject.m_fLuxLighted - gs_sMeasPartObject.m_fLuxZero;
        timeInSeconds = (float) gs_sMeasPartObject.m_uMillisCounter/1000;
        // Print light data and temperature
        printf("%.3f\t%u\t%u\t%.3f\t%u\t%u\t%.3f\t%.1f\t%.3f\r\n", gs_sMeasPartObject.m_fLuxLighted,gs_sMeasPartObject.m_uFullLighted, gs_sMeasPartObject.m_uIrLighted, gs_sMeasPartObject.m_fLuxDark,gs_sMeasPartObject.m_uFullDark, gs_sMeasPartObject.m_uIrDark, gs_sMeasPartObject.m_fTemperatureCelcius,(float)(.33*gs_sMeasPartObject.adcValue/4.096), timeInSeconds);
    }

     
}

uint8_t measureParticlesGetDacValue(void)
{
    return gs_sMeasPartObject.dacValue;
}

void measureParticlesSetDacValue(uint8_t value)
{
    gs_sMeasPartObject.dacValue = value;
    DAC1_SetOutput(gs_sMeasPartObject.dacValue);
}

void MeasureTimerInterrupt(void)
{
    gs_sMeasPartObject.m_uMillisCounter++;
}

void measureSetLED(float currentSetting)
{
    uint16_t adcRequest;
    adc_result_t shunt;
    uint32_t adcMeasured = 0;
    uint16_t adc_memory = 0;
    bool upMotion = false;
    uint16_t N,i;
    
    DAC1_Enable();
    
    adcRequest = (uint16_t) (4096*currentSetting/3.3);
    //shunt = ADCC_GetSingleConversion(channel_ANB7);
    
    while(adcMeasured != adcRequest)
    {
        adcMeasured = 0;
        N = 6;
        i = 0;
        while(i< N)
        {
            if(ADCC_IsConversionDone())
            {
                shunt = ADCC_GetSingleConversion(channel_ANB7);
                adcMeasured += shunt;
                i++;
            }
            
        }
        adcMeasured = adcMeasured/N;
        
        if(adcMeasured > adcRequest)
        {
            if(adcMeasured - adcRequest > 10)
            {
                if(upMotion && (adcRequest - adc_memory) < (adcMeasured - adcRequest))
                {
                    break;
                }
                upMotion = false;

                adc_memory = adcMeasured;
                //printf("down %u \r\n", adcMeasured);
                gs_sMeasPartObject.dacValue = gs_sMeasPartObject.dacValue - 1;
                DAC1_SetOutput(gs_sMeasPartObject.dacValue);    
            }
            else
            {
                break;
            }
        }
        else
        {
            if(adcRequest - adcMeasured > 10)
            {
                if(!upMotion && (adcRequest - adcMeasured) < (adc_memory - adcMeasured))
                {
                    break;
                }
                upMotion = true;
                adc_memory = adcMeasured;
                //printf("up %u \r\n", adcMeasured);

                gs_sMeasPartObject.dacValue = gs_sMeasPartObject.dacValue - 1;
                DAC1_SetOutput(gs_sMeasPartObject.dacValue);
            }
            else
            {
                break;
            }
        }
        //shunt = ADCC_GetSingleConversion(channel_ANB7);
    }
    gs_sMeasPartObject.adcValue = (uint16_t) shunt;
    DAC1_Disable();
}

