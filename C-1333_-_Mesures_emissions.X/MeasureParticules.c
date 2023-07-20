/*
Solutions Novika.
Copyright 2021
===========================================================================

Filename :      MeasureParticles.c

Author(s):      Clement Soucy, CPI # 6027411
 *              Charles Richard, CPI # 6045522

Public prefix : measureParticles

Project # : C-1333, C-1421

Product: Mesure d'Ã©missions polluantes

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
#include "ParameterFile.h"
#include "TSL2591.h"
#include "DS1775.h"
#include "GPIO.h"
#include "mcc_generated_files/tmr0.h"
#include "mcc_generated_files/dac1.h"
#include "mcc_generated_files/adcc.h"
#include "ControlBridge.h"
#include "MeasureParticles.h"


#define LED_ON 1
#define LED_OFF 0

static SMeasureParticlesObject gs_sMeasPartObject;

void measureParticlesSenseCompleteCallback(void);
void measureParticlesPrintData(void);
void MeasureTimerInterrupt(void);
void measureSetLED(const gs_Parameters* Param);

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
    
    gs_sMeasPartObject.m_fullZero = 0;
    gs_sMeasPartObject.m_uLastRead = 0;    

    gs_sMeasPartObject.adcValue = 0;
    
    TMR0_SetInterruptHandler(MeasureTimerInterrupt); //millisecond counter
    TMR0_StartTimer();
    
}

void measureParticlesProcess(void)
{
    const gs_Parameters* MP_Param = PF_getCBParamAddr();
    switch (gs_sMeasPartObject.m_eState)
    {
        case eMEASURE_PARTICLES_NOT_STARTED:
        {
            if (tsl2591IsStarted() && ds1775IsStarted()) // give time to drivers to configure the ICs at startup
            {
                //DAC1_SetOutput(MP_Param->DAC_value); //Set DAC to arbitrary value
                measureSetLED(MP_Param); // Configure DAC value to approach Led current requested value
                ds1775RequestRead(); 
                gs_sMeasPartObject.m_eState = eMEASURE_PARTICLES_REQUEST_NOT_SENT;
            }
            break;
        }
        case eMEASURE_PARTICLES_REQUEST_NOT_SENT:
        {
            if(MP_Param->AcqEnable)
            {
                if (gs_sMeasPartObject.m_eSubState == eSUB_STATE_DARK && (gs_sMeasPartObject.m_uMillisCounter - gs_sMeasPartObject.m_uLastRead) > (MP_Param->MeasureInterval))
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
                    
                    // Check if driver is ready
                    if (tsl2591IsReadyForRequest())
                    {
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
                DAC1_Enable();
                
                // Change state and sub state
                gs_sMeasPartObject.m_eSubState = eSUB_STATE_LIGHTED;
                gs_sMeasPartObject.m_eState = eMEASURE_PARTICLES_REQUEST_NOT_SENT;
            }
            else if (gs_sMeasPartObject.m_eSubState == eSUB_STATE_LIGHTED)
            {
                // measure current in LED
                ADCC_DischargeSampleCapacitor();
                gs_sMeasPartObject.adcValue = (uint16_t) ADCC_GetSingleConversion(channel_ANB7);
                
                // De-activate LED
                DAC1_Disable();
                
                // Change state and sub state
                gs_sMeasPartObject.m_eSubState = eSUB_STATE_DARK;
                gs_sMeasPartObject.m_eState = eMEASURE_PARTICLES_UPDATE_BRIDGE;
                
                // Notify control bridge new data is available
                bridgeDataRDY();
                
                // print data in console if it is enabled
                gs_sMeasPartObject.m_uLastRead = gs_sMeasPartObject.m_uMillisCounter;
                if (MP_Param->PrintEnable)
                {    
                    measureParticlesPrintData();
                }
                
            }
            else if (gs_sMeasPartObject.m_eSubState == eSUB_STATE_ZERO)
            {
                // measure current in LED
                ADCC_DischargeSampleCapacitor();
                gs_sMeasPartObject.adcValue = (uint16_t) ADCC_GetSingleConversion(channel_ANB7);
                
                // De-activate LED
                DAC1_Disable();
                
                // Write zero values in EEPROM parameters (wait for ctrl card to actually save in EEPROM)   
                PF_Update_MemParams(PF_GetFireCount(),gs_sMeasPartObject.m_fullZero,(uint8_t)(3300*(float)gs_sMeasPartObject.adcValue/4096),(uint8_t)gs_sMeasPartObject.m_fTemperatureCelcius);
                bridgeZeroComplete();

                // Change state and sub state
                gs_sMeasPartObject.m_eSubState = eSUB_STATE_DARK;
                gs_sMeasPartObject.m_eState = eMEASURE_PARTICLES_REQUEST_NOT_SENT;
            }
            break;
        }
        case eMEASURE_PARTICLES_SET_ZERO:
        {
            // activate LED
            DAC1_Enable();
            ds1775RequestRead(); 
                
            // Change state and sub state
            gs_sMeasPartObject.m_eSubState = eSUB_STATE_ZERO;

            if (tsl2591IsReadyForRequest())
            {
                tsl2591SenseLight(&gs_sMeasPartObject.m_fullZero, &gs_sMeasPartObject.m_uIrLighted, &gs_sMeasPartObject.m_fLuxLighted, measureParticlesSenseCompleteCallback);
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
            if(tsl2591IsReadyForRequest())
            {
                TSLreset();
                ds1775Reset();
                DAC1_SetOutput(MP_Param->DAC_value);
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
    return gs_sMeasPartObject.m_eState == eMEASURE_PARTICLES_REQUEST_NOT_SENT;
}

void measureReset(void)
{
    gs_sMeasPartObject.m_eState = eMEASURE_PARTICLES_RECONFIGURE;
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
    float timeInSeconds;
    
    timeInSeconds = (float) gs_sMeasPartObject.m_uMillisCounter/1000;
    // Print light data and temperature
    printf("%.3f\t%u\t%u\t%.3f\t%u\t%u\t%.3f\t%.1f\t%.3f\r\n",
            gs_sMeasPartObject.m_fLuxLighted,gs_sMeasPartObject.m_uFullLighted,
            gs_sMeasPartObject.m_uIrLighted, gs_sMeasPartObject.m_fLuxDark,
            gs_sMeasPartObject.m_uFullDark, gs_sMeasPartObject.m_uIrDark,
            gs_sMeasPartObject.m_fTemperatureCelcius,(3300*(float)gs_sMeasPartObject.adcValue/4096),
            timeInSeconds);
}

void measureParticlesSetDacValue(const gs_Parameters* Param)
{
    DAC1_SetOutput(Param->DAC_value);
}

void MeasureTimerInterrupt(void)
{
    gs_sMeasPartObject.m_uMillisCounter++;
}

void measureSetLED(const gs_Parameters* Param)
{
    uint16_t adcRequest;
    adc_result_t shunt;
    uint32_t adcMeasured = 0;
    uint16_t adc_memory = 0;
    bool upMotion = false;
    uint16_t N,i;
    
    uint8_t DAC = Param->DAC_value;
    
    DAC1_Enable();
    
    adcRequest = (uint16_t) (4096*Param->Current_cmd/330);
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
                DAC -= 1;
                DAC1_SetOutput(DAC);    
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

                DAC += 1;
                DAC1_SetOutput(DAC);
            }
            else
            {
                break;
            }
        }
        //shunt = ADCC_GetSingleConversion(channel_ANB7);
    }
    PF_setDAC(DAC);
    gs_sMeasPartObject.adcValue = (uint16_t) shunt;
    DAC1_Disable();
}