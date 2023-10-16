/*
Solutions Novika.
Copyright 2023
===========================================================================

Filename :      ParameterFile.c

Author(s):      Charles Richard, CPI # 6045522

Public prefix : parameterFile

Project # : C-1421

Product: Mesure d'émissions polluantes

Creation date:  2021/06/22

Description:    Container for global function parameters.

===========================================================================

Modifications
-------------

By    |   Date     | Version | Description
------+------------+---------+---------------------------------------------
CR    | 2023/02/06 | -       | Creation
===========================================================================
*/ 

#include "ParameterFile.h"
#include "mcc_generated_files/memory.h"

#define EE_FIRST_CONF_ADDR  0x00
#define EE_FIRE_COUNTER_MSB 0x01
#define EE_FIRE_COUNTER_LSB 0x02
#define EE_LAST_ZERO_MSB    0x03
#define EE_LAST_ZERO_LSB    0x04
#define EE_CURRENT_ADDR     0x05
#define EE_DAC_ADDR         0x06
#define EE_TEMP_ADDR        0x07

static gs_Parameters Params;
static gs_MemoryParams EEParams;

void PF_Update_EEPROM(void);

void ParameterInit(void)
{
    Params.TSL_gain = TSL2591_GAIN_MAX;
    Params.TSL_integrationTime = TSL2591_INTEGRATIONTIME_600MS;
    Params.DS1775_resolution = DS1775_11BITS_MODE;
    Params.MeasureInterval = 1600;
    Params.PrintEnable = false;
    Params.AcqEnable = true;
    Params.Current_cmd = 5.3;
    //Params.DAC_value = 140;
    if(DATAEE_ReadByte(EE_FIRST_CONF_ADDR) != 0xAA)
    {
        DATAEE_WriteByte(EE_FIRST_CONF_ADDR, 0xAA);
        EEParams.fireCounter = 0;
        Params.DAC_value = 130;//Hard coded value --> gives roughly 5 mA (best go through void measureSetLED(const gs_Parameters* Param))
    }else
    {
        Params.DAC_value = DATAEE_ReadByte(EE_DAC_ADDR);
        EEParams.fireCounter = (uint16_t)((DATAEE_ReadByte(EE_FIRE_COUNTER_MSB) << 8) + DATAEE_ReadByte(EE_FIRE_COUNTER_LSB));
        EEParams.lastZeroValue = (uint16_t)((DATAEE_ReadByte(EE_LAST_ZERO_MSB) << 8) + DATAEE_ReadByte(EE_LAST_ZERO_LSB));
        EEParams.lastZeroCurrent = DATAEE_ReadByte(EE_CURRENT_ADDR);
        EEParams.lastZeroTemp = DATAEE_ReadByte(EE_TEMP_ADDR);
        EEParams.lastZeroDac = Params.DAC_value;
        
    }
    
}

void PF_IncrementFireCount(void) // How many fires monitored since installation
{
    uint16_t count = (uint16_t)((DATAEE_ReadByte(EE_FIRE_COUNTER_MSB) << 8) + DATAEE_ReadByte(EE_FIRE_COUNTER_LSB));
    count++;
    DATAEE_WriteByte(EE_FIRE_COUNTER_MSB, (uint8_t)(count >> 8));
    DATAEE_WriteByte(EE_FIRE_COUNTER_LSB, (uint8_t)(count & 0x00FF));
    
    EEParams.fireCounter = count;     
}

uint16_t PF_GetFireCount(void)
{
    return (uint16_t)((DATAEE_ReadByte(EE_FIRE_COUNTER_MSB) << 8) + DATAEE_ReadByte(EE_FIRE_COUNTER_LSB));
}

void PF_Update_MemParams(uint16_t FireCount, uint16_t zero, uint8_t current, uint8_t temperature)  
{
    EEParams.fireCounter = FireCount;
    EEParams.lastZeroValue = zero;
    EEParams.lastZeroCurrent = current;
    EEParams.lastZeroTemp = temperature;
    EEParams.lastZeroDac = Params.DAC_value;    
}


void PF_Update_EEPROM(void) // Save memorizable paramters to EEPROM  //GC 2023-07-19 N'est pas appelé.  
{
    DATAEE_WriteByte(EE_FIRE_COUNTER_MSB, (uint8_t)(EEParams.fireCounter >> 8));
    DATAEE_WriteByte(EE_FIRE_COUNTER_LSB, (uint8_t)(EEParams.fireCounter & 0x00FF));
    DATAEE_WriteByte(EE_LAST_ZERO_MSB, (uint8_t)(EEParams.lastZeroValue >> 8));
    DATAEE_WriteByte(EE_LAST_ZERO_LSB, (uint8_t)(EEParams.lastZeroValue & 0x00FF));
    DATAEE_WriteByte(EE_CURRENT_ADDR, EEParams.lastZeroCurrent);
    DATAEE_WriteByte(EE_DAC_ADDR, EEParams.lastZeroDac);
    DATAEE_WriteByte(EE_TEMP_ADDR, EEParams.lastZeroTemp);
}

void PF_Read_EE_Param(gs_MemoryParams *MemParam)  //À tester
{
    MemParam->fireCounter = (uint16_t)((DATAEE_ReadByte(EE_FIRE_COUNTER_MSB) << 8) + DATAEE_ReadByte(EE_FIRE_COUNTER_LSB));
    MemParam->lastZeroCurrent = DATAEE_ReadByte(EE_CURRENT_ADDR);
    MemParam->lastZeroDac = DATAEE_ReadByte(EE_DAC_ADDR);
    MemParam->lastZeroTemp = DATAEE_ReadByte(EE_TEMP_ADDR);
    MemParam->lastZeroValue = (uint16_t)((DATAEE_ReadByte(EE_LAST_ZERO_MSB) << 8) + DATAEE_ReadByte(EE_LAST_ZERO_LSB));   
}

void PF_requestReconfigure(void) // Stop acquisition to change configuration
{
    Params.AcqEnable = false;
}

void PF_ReConfigureDone(void) // ReStart acquisition after configuration change
{
    Params.AcqEnable = true;
}

void PF_ToggleAcqEnable(void)
{
    Params.AcqEnable = !Params.AcqEnable;
}

void PF_setDAC(uint8_t value)
{
    Params.DAC_value = value;
}

bool PF_validateConfig(uint8_t* payload, uint8_t payload_size)
{
    if(payload_size == 4) // Hard coded value, if more parameters are needed, put theme in the loop and update the size
    {
        if(payload[0] < 4) //GC - Ne pas mettre de "magic number" Définir pourquoi ces limitation
        {
            if(payload[1] < 6) //GC - Ne pas mettre de "magic number" Définir pourquoi ces limitation
            {
                if(payload[3] < 11)  //GC - Ne pas mettre de "magic number" Définir pourquoi ces limitation
                {
                    Params.TSL_gain = payload[0];
                    Params.TSL_integrationTime = payload[1];
                    Params.DAC_value = payload[2];
                    Params.MeasureInterval = payload[3];
                    return true;
                }
            }
        } 
    }
    return false;
}

const gs_Parameters* PF_getCBParamAddr(void){
    return &Params;
}

const gs_MemoryParams* PF_getEEParamAddr(void){
    return &EEParams;
}
