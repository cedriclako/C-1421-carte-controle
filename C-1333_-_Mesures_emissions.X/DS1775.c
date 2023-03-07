
/*
Solutions Novika.
Copyright 2021
===========================================================================

Filename :      DS1775.h

Author(s):      Clement Soucy, CPI # 6027411
 *              Charles Richard, CPI # 6045522

Public prefix : ds1775

Project # : C-1333, C-1421

Product: Mesure d'émissions polluantes

Creation date:  2021/06/21

Description:    DS1775 temperature sensor driver.

===========================================================================

Modifications
-------------

By    |   Date     | Version | Description
------+------------+---------+---------------------------------------------
CS    | 2021/06/21 | -       | Creation
CR    | 2022/10/14 | 0.1     | Corrections
===========================================================================
*/ 

#include "ParameterFile.h"
#include "DS1775.h"
#include "I2CEngine.h"


#define DS1775_ADDR 0x4C

//static gs_Parameters* tempParam;


enum{
    
    DS1775_TEMPERATURE_ADDRESS = 0x00,
    DS1775_CONFIGURATION_ADDRESS = 0x01,
    DS1775_THYST_ADDRESS = 0x02,
    DS1775_TOS_ADDRESS = 0x03,
};

typedef enum
{
    eDS1775_NOT_CONFIGURED = 0,
    eDS1775_CONFIGURING,
    eDS1775_IDLE,
    eDS1775_READ_REQUIRED,
    eDS1775_READING,
    eDS1775_DATA_RECEIVED,

    eDS1775_NB_STATES,
}EDS1775States;

typedef struct
{
    uint16_t m_uTemperatureRegister;
    float m_fTemperatureCelcius;
    bool m_bDataReady;
    EDS1775States m_eState;
    
}SDS1775Object;

static SDS1775Object gs_sDS1775Object;

static uint8_t gs_uReadBufferDS1775[2];
static uint8_t gs_uWriteBufferDS1775;
static uint8_t gs_uRegDS1775;
static bool gs_bFirstRead = true;


void ds1775SuccessCallback(void);
void ds1775ErrorCallback(void);
void ds1775ConvertToCelcius(void);
void ds1775PointerToTempSuccess(void);


void ds1775Initialize(void)
{
    gs_sDS1775Object.m_bDataReady = false;
    gs_sDS1775Object.m_eState = eDS1775_NOT_CONFIGURED;
    gs_sDS1775Object.m_fTemperatureCelcius = 0.0;
    gs_sDS1775Object.m_uTemperatureRegister = 0;
}

bool ds1775IsDataReady(void)
{
    return gs_sDS1775Object.m_bDataReady;
}

float ds1775GetTemperatureCelcius(void)
{
    gs_sDS1775Object.m_bDataReady = false;
    return gs_sDS1775Object.m_fTemperatureCelcius;
}

bool ds1775IsStarted(void)
{
    return ((gs_sDS1775Object.m_eState != eDS1775_NOT_CONFIGURED) && (gs_sDS1775Object.m_eState != eDS1775_CONFIGURING));
}

void ds1775ConvertToCelcius(void)
{
    uint16_t uRegVal = gs_sDS1775Object.m_uTemperatureRegister;
    int8_t iSign = 1;
    uint8_t uIntegerPart;
    float fDecimalPart;
    
    
    if (uRegVal & 0x8000) // Temperature is negative
    {
        iSign = -1;
        
        // take two's complement
        uRegVal ^= 0xFFFF;
        
        // Discard padding zeros
        uRegVal = (uRegVal >> 4);
        
        // Add 1
        uRegVal ++;
        
        // Add padding zeros
        uRegVal = ((uRegVal << 4) & 0xFFF0);
    }
    
    // Isolate integer part
    uIntegerPart = (uRegVal >> 8);
    
    // Isolate decimal part
    fDecimalPart = (float) ((((uRegVal & 0x0080) >> 7) * 0.5) + (((uRegVal & 0x0040) >> 6) * 0.25) + (((uRegVal & 0x0020) >> 5) * 0.125) + (((uRegVal & 0x0010) >> 4) * 0.0625));
    
    gs_sDS1775Object.m_fTemperatureCelcius = (uIntegerPart + fDecimalPart) * iSign;
}

void ds1775Process(void)
{
    const gs_Parameters* tempParam = PF_getCBParamAddr();
  
    switch (gs_sDS1775Object.m_eState)
    {
        case eDS1775_NOT_CONFIGURED:
        {
            if (i2cIsBusIdle())
            {
                gs_uWriteBufferDS1775 = tempParam->DS1775_resolution;
                gs_uRegDS1775 = DS1775_CONFIGURATION_ADDRESS;
                
                i2cWrite(&gs_uWriteBufferDS1775, 1, DS1775_ADDR, gs_uRegDS1775, ds1775SuccessCallback, ds1775ErrorCallback);
                gs_sDS1775Object.m_eState = eDS1775_CONFIGURING;
                
            }
            break;
        }
        case eDS1775_CONFIGURING:
        {
            // Wait success / error callback
            break;
        }
        case eDS1775_IDLE:
        {
            // Wait the end of timer to change to state eDS1775_READ_REQUIRED
            break;
        }
        case eDS1775_READ_REQUIRED:
        {
            if(gs_bFirstRead)
            {
                ds1775SetPointerToTemp();
            }
                if (i2cIsBusIdle())
                {
                    gs_uRegDS1775 = DS1775_TEMPERATURE_ADDRESS;
                    i2cRead(gs_uReadBufferDS1775, 2, DS1775_ADDR, gs_uRegDS1775, ds1775SuccessCallback, ds1775ErrorCallback);
                    
                    gs_sDS1775Object.m_eState = eDS1775_READING;
                    
                }
            

            break;
        }
        case eDS1775_READING:
        {
            // Wait success / error callback
            break;
        }
        case eDS1775_DATA_RECEIVED:
        {
            gs_sDS1775Object.m_uTemperatureRegister = ((uint16_t) gs_uReadBufferDS1775[0] << 8) + ((uint16_t) gs_uReadBufferDS1775[1]);
            ds1775ConvertToCelcius();
            gs_sDS1775Object.m_bDataReady = true;
            gs_sDS1775Object.m_eState = eDS1775_IDLE;
            
            break;
        }
        default:
        {
            break;
        }
    }
}

void ds1775SuccessCallback(void)
{
    switch (gs_sDS1775Object.m_eState)
    {
        case eDS1775_NOT_CONFIGURED:
        {
            // No transmission in this state
            break;
        }
        case eDS1775_CONFIGURING:
        {    
            gs_sDS1775Object.m_eState = eDS1775_IDLE;         
            break;
        }
        case eDS1775_IDLE:
        {
            // No transmission in this state
            break;
        }
        case eDS1775_READ_REQUIRED:
        {
            // No transmission in this state
            break;
        }
        case eDS1775_READING:
        {
            gs_sDS1775Object.m_eState = eDS1775_DATA_RECEIVED;
            break;
        }
        case eDS1775_DATA_RECEIVED:
        {
            // No transmission in this state
            break;
        }
        default:
        {
            break;
        }
    }
}

void ds1775ErrorCallback(void)
{
    switch (gs_sDS1775Object.m_eState)
    {
        case eDS1775_NOT_CONFIGURED:
        {
            // No transmission in this state
            break;
        }
        case eDS1775_CONFIGURING:
        {
            gs_sDS1775Object.m_eState = eDS1775_NOT_CONFIGURED;
            break;
        }
        case eDS1775_IDLE:
        {
            // No transmission in this state
            break;
        }
        case eDS1775_READ_REQUIRED:
        {
            // No transmission in this state
            break;
        }
        case eDS1775_READING:
        {
            gs_sDS1775Object.m_eState = eDS1775_READ_REQUIRED;
            break;
        }
        case eDS1775_DATA_RECEIVED:
        {
            // No transmission in this state
            break;
        }
        default:
        {
            break;
        }
    }
}

void ds1775SetPointerToTemp(void)
{
    if (i2cIsBusIdle())
    {
        gs_uRegDS1775 = DS1775_TEMPERATURE_ADDRESS;
        i2cWrite(&gs_uWriteBufferDS1775, 0, DS1775_ADDR, gs_uRegDS1775, ds1775PointerToTempSuccess, NULL);
        
    }
    
}

void ds1775PointerToTempSuccess(void)
{
    gs_bFirstRead = false; 
}

void ds1775RequestRead(void)
{
    gs_sDS1775Object.m_eState = eDS1775_READ_REQUIRED;
}

void ds1775Reset(void)
{
    gs_sDS1775Object.m_eState = eDS1775_NOT_CONFIGURED;
}