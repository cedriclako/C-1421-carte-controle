/* 
 * File:   ParameterFile.h
 * Author: crichard
 *
 * Created on February 6, 2023, 9:19 AM
 */

#ifndef PARAMETERFILE_H
#define	PARAMETERFILE_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include <xc.h>
#include <stdbool.h>
    
    /// Enumeration for the sensor gain
    typedef enum {
        TSL2591_GAIN_LOW = 0x00,  /// low gain (1x)
        TSL2591_GAIN_MED = 0x10,  /// medium gain (25x)
        TSL2591_GAIN_HIGH = 0x20, /// medium gain (428x)
        TSL2591_GAIN_MAX = 0x30,  /// max gain (9876x)
    } tsl2591Gain_t;
    
    /// Enumeration for the sensor integration timing
    typedef enum {
        TSL2591_INTEGRATIONTIME_100MS = 0x00, // 100 millis
        TSL2591_INTEGRATIONTIME_200MS = 0x01, // 200 millis
        TSL2591_INTEGRATIONTIME_300MS = 0x02, // 300 millis
        TSL2591_INTEGRATIONTIME_400MS = 0x03, // 400 millis
        TSL2591_INTEGRATIONTIME_500MS = 0x04, // 500 millis
        TSL2591_INTEGRATIONTIME_600MS = 0x05, // 600 millis
    } tsl2591IntegrationTime_t;
    
    #define DS1775_9BITS_MODE 0x00      // Conversion time: 125ms typ. 187.5ms max
    #define DS1775_10BITS_MODE 0x20     // Conversion time: 250ms typ. 375ms max
    #define DS1775_11BITS_MODE 0x40     // Conversion time: 500ms typ. 750ms max
    #define DS1775_12BITS_MODE 0x60     // Conversion time: 1000ms typ. 1500ms max

    typedef struct
    {
        // Light measurement parameters
        tsl2591IntegrationTime_t TSL_integrationTime;
        tsl2591Gain_t TSL_gain;
        
        // Temperature monitoring precision (9 - 12 bits)
        uint8_t DS1775_resolution;
        
        // Current control (through LEDs)
        
        uint8_t DAC_value; // Actual value in command
        float Current_cmd;  // User defined --> if defined, modify DAC value to get as close to cmd as possible
        
        // Global device configs
        
        uint16_t MeasureInterval; // How long between light acquisitions
        bool PrintEnable;
        bool AcqEnable;
        bool ReconfigurationInProgress;        
    }gs_Parameters;
    
    typedef struct
    {
        uint16_t fireCounter;
        uint16_t lastZeroValue;
        uint8_t lastZeroCurrent;
        uint8_t lastZeroDac;
        uint8_t lastZeroTemp;
    } gs_MemoryParams;
    
    
    void ParameterInit(void);
    
    void PF_requestReconfigure(void);
    
    void PF_ToggleAcqEnable(void);
    
    void PF_setDAC(uint8_t value);
    
    void PF_Update_MemParams(uint16_t zero, uint8_t current,uint8_t temperature);
    
    bool PF_validateConfig(uint8_t* payload,uint8_t payload_size);
    
    void PF_IncrementFireCount(void);
    
    const gs_Parameters* PF_getCBParamAddr(void);
    
    const gs_MemoryParams* PF_getEEParamAddr(void);


#ifdef	__cplusplus
}
#endif

#endif	/* PARAMETERFILE_H */

