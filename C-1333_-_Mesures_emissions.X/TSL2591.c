/*
Solutions Novika.
Copyright 2021
===========================================================================

Filename :      TSL2591.c

Author(s):      Clement Soucy, CPI # 6027411
 *              Charles Richard, CPI # 6045522

Public prefix : tsl2591

Project # : C-1333, C-1421

Product: Mesure d'émissions polluantes

Creation date:  2021/06/18

Description:    TSL2591 driver.

===========================================================================

Modifications
-------------

By    |   Date     | Version | Description
------+------------+---------+---------------------------------------------
CS    | 2021/06/18 | -       | Creation
CR    | 2022/10/14 | 0.1     | Corrections
===========================================================================
*/ 

#include "TSL2591.h"
#include "I2CEngine.h"
#include "mcc_generated_files/mcc.h"
// =============================================================================
//  From Adafruit
// =============================================================================
#define TSL2591_VISIBLE (2)      ///< (channel 0) - (channel 1)
#define TSL2591_INFRARED (1)     ///< channel 1
#define TSL2591_FULLSPECTRUM (0) ///< channel 0

#define TSL2591_ADDR (0x29) ///< Default I2C address

#define TSL2591_COMMAND_BIT (0xA0) ///< 1010 0000: bits 7 and 5 for 'command normal'

///! Special Function Command for "Clear ALS and no persist ALS interrupt"
#define TSL2591_CLEAR_INT (0xE7)
///! Special Function Command for "Interrupt set - forces an interrupt"
#define TSL2591_TEST_INT (0xE4)

#define TSL2591_WORD_BIT (0x20)  ///< 1 = read/write word (rather than byte)
#define TSL2591_BLOCK_BIT (0x10) ///< 1 = using block read/write

#define TSL2591_ENABLE_POWEROFF (0x00) ///< Flag for ENABLE register to disable
#define TSL2591_ENABLE_POWERON (0x01)  ///< Flag for ENABLE register to enable
#define TSL2591_ENABLE_AEN  (0x02) 
        ///< ALS Enable. This field activates ALS function. Writing a one
        ///< activates the ALS. Writing a zero disables the ALS.
#define TSL2591_ENABLE_AIEN (0x10) 
        ///< ALS Interrupt Enable. When asserted permits ALS interrupts to be
        ///< generated, subject to the persist filter.
#define TSL2591_ENABLE_NPIEN    (0x80) 
        ///< No Persist Interrupt Enable. When asserted NP Threshold conditions
        ///< will generate an interrupt, bypassing the persist filter

#define TSL2591_LUX_DF (408.0F)   ///< Lux cooefficient
#define TSL2591_LUX_COEFB (1.64F) ///< CH0 coefficient
#define TSL2591_LUX_COEFC (0.59F) ///< CH1 coefficient A
#define TSL2591_LUX_COEFD (0.86F) ///< CH2 coefficient B

/// TSL2591 Register map
enum {
  TSL2591_REGISTER_ENABLE = 0x00,          // Enable register
  TSL2591_REGISTER_CONTROL = 0x01,         // Control register
  TSL2591_REGISTER_THRESHOLD_AILTL = 0x04, // ALS low threshold lower byte
  TSL2591_REGISTER_THRESHOLD_AILTH = 0x05, // ALS low threshold upper byte
  TSL2591_REGISTER_THRESHOLD_AIHTL = 0x06, // ALS high threshold lower byte
  TSL2591_REGISTER_THRESHOLD_AIHTH = 0x07, // ALS high threshold upper byte
  TSL2591_REGISTER_THRESHOLD_NPAILTL = 0x08, // No Persist ALS low threshold lower byte
  TSL2591_REGISTER_THRESHOLD_NPAILTH = 0x09, // No Persist ALS low threshold higher byte
  TSL2591_REGISTER_THRESHOLD_NPAIHTL = 0x0A, // No Persist ALS high threshold lower byte
  TSL2591_REGISTER_THRESHOLD_NPAIHTH = 0x0B, // No Persist ALS high threshold higher byte
  TSL2591_REGISTER_PERSIST_FILTER = 0x0C, // Interrupt persistence filter
  TSL2591_REGISTER_PACKAGE_PID = 0x11,    // Package Identification
  TSL2591_REGISTER_DEVICE_ID = 0x12,      // Device Identification
  TSL2591_REGISTER_DEVICE_STATUS = 0x13,  // Internal Status
  TSL2591_REGISTER_CHAN0_LOW = 0x14,      // Channel 0 data, low byte
  TSL2591_REGISTER_CHAN0_HIGH = 0x15,     // Channel 0 data, high byte
  TSL2591_REGISTER_CHAN1_LOW = 0x16,      // Channel 1 data, low byte
  TSL2591_REGISTER_CHAN1_HIGH = 0x17,     // Channel 1 data, high byte
};

/// Enumeration for the sensor integration timing
typedef enum {
  TSL2591_INTEGRATIONTIME_100MS = 0x00, // 100 millis
  TSL2591_INTEGRATIONTIME_200MS = 0x01, // 200 millis
  TSL2591_INTEGRATIONTIME_300MS = 0x02, // 300 millis
  TSL2591_INTEGRATIONTIME_400MS = 0x03, // 400 millis
  TSL2591_INTEGRATIONTIME_500MS = 0x04, // 500 millis
  TSL2591_INTEGRATIONTIME_600MS = 0x05, // 600 millis
} tsl2591IntegrationTime_t;

/// Enumeration for the persistance filter (for interrupts)
typedef enum {
  //  bit 7:4: 0
  TSL2591_PERSIST_EVERY = 0x00, // Every ALS cycle generates an interrupt
  TSL2591_PERSIST_ANY = 0x01,   // Any value outside of threshold range
  TSL2591_PERSIST_2 = 0x02,     // 2 consecutive values out of range
  TSL2591_PERSIST_3 = 0x03,     // 3 consecutive values out of range
  TSL2591_PERSIST_5 = 0x04,     // 5 consecutive values out of range
  TSL2591_PERSIST_10 = 0x05,    // 10 consecutive values out of range
  TSL2591_PERSIST_15 = 0x06,    // 15 consecutive values out of range
  TSL2591_PERSIST_20 = 0x07,    // 20 consecutive values out of range
  TSL2591_PERSIST_25 = 0x08,    // 25 consecutive values out of range
  TSL2591_PERSIST_30 = 0x09,    // 30 consecutive values out of range
  TSL2591_PERSIST_35 = 0x0A,    // 35 consecutive values out of range
  TSL2591_PERSIST_40 = 0x0B,    // 40 consecutive values out of range
  TSL2591_PERSIST_45 = 0x0C,    // 45 consecutive values out of range
  TSL2591_PERSIST_50 = 0x0D,    // 50 consecutive values out of range
  TSL2591_PERSIST_55 = 0x0E,    // 55 consecutive values out of range
  TSL2591_PERSIST_60 = 0x0F,    // 60 consecutive values out of range
} tsl2591Persist_t;

/// Enumeration for the sensor gain
typedef enum {
  TSL2591_GAIN_LOW = 0x00,  /// low gain (1x)
  TSL2591_GAIN_MED = 0x10,  /// medium gain (25x)
  TSL2591_GAIN_HIGH = 0x20, /// medium gain (428x)
  TSL2591_GAIN_MAX = 0x30,  /// max gain (9876x)
} tsl2591Gain_t;

// =============================================================================
//  Adapted code to PIC MCU below
// =============================================================================

typedef void (* tsl2591Callback) (void);

void tsl2591SetRegisterPointer(uint8_t reg);
void tsl2591RegisterPointerSuccess(void);
void tsl2591SuccessCallback(void);
void tsl2591ErrorCallback(void);

void tsl2591ReturnValues(void);
void tsl2591CalculateLux(void);

typedef enum
{
    eTSL2591_NOT_CONFIGURED = 0,    // At startup
    eTSL2591_CONFIGURING,       // Sending gain and integration time
    eTSL2591_ALS_IDLE,          // Waiting request to read light
    eTSL2591_REQUEST_RECEIVED,  // Need to start ALS conversions      
    eTSL2591_ENABLING,          // Enabling ALS (PON and AEN)
    eTSL2591_ENABLED,           // ALS is enabled and need to start a read to AVALID bit
    eTSL2591_POLLING_AVALID,    // Check when data is ready
    eTSL2591_DATA_READY,        // Data is ready, need to start a read
    eTSL2591_READING_RAW,       // Retrieving information
    eTSL2591_DATA_RECEIVED,     // Need to copy data in output variable, compute Lux and call user callback
    eTLS2591_DISABLE_ALS,       // Need to disable ALS
    eTSL2591_DISABLING,         // Disabling ALS to reset cycle (AEN)
            
    eTSL2591_NB_STATES,
}ETSL2591States;

typedef struct
{
    ETSL2591States m_eState;
    uint32_t m_uRawFull32;
    uint16_t m_uRawFull16;
    uint16_t m_uRawIr16;
    float m_fLux;
    tsl2591Gain_t m_eGain;
    tsl2591IntegrationTime_t m_eTime;
    
    
    uint16_t* m_pFullOutputVal;
    uint16_t* m_pIrOutputVal;
    float* m_pLuxOuputVal;
    tsl2591Callback m_Callback;
    
}STSL2591Object;

static STSL2591Object gs_sTSL2591Object;
static uint8_t gs_uWriteBuffer;
static uint8_t gs_uReg;
static uint8_t gs_uReadBuffer[4]; 
static uint8_t gs_uTimerCounter;
static uint8_t gs_uRegisterPointer = 0;


void tsl2591Initialize(void)
{
    gs_sTSL2591Object.m_Callback = NULL;
    gs_sTSL2591Object.m_eState = eTSL2591_NOT_CONFIGURED;
    gs_sTSL2591Object.m_uRawFull32 = 0;
    gs_sTSL2591Object.m_uRawFull16 = 0;
    gs_sTSL2591Object.m_uRawIr16 = 0;
    gs_sTSL2591Object.m_eGain = TSL2591_GAIN_MAX;
    gs_sTSL2591Object.m_eTime = TSL2591_INTEGRATIONTIME_600MS;
    
}

void tsl2591Process(void)
{
    switch (gs_sTSL2591Object.m_eState)
    {
        case eTSL2591_NOT_CONFIGURED:
        {
            // Try to configure
            if (i2cIsBusIdle())
            {

                gs_uWriteBuffer = (gs_sTSL2591Object.m_eGain | gs_sTSL2591Object.m_eTime);
                gs_uReg = (TSL2591_COMMAND_BIT | TSL2591_REGISTER_CONTROL);
                
                i2cWrite(&gs_uWriteBuffer, 1, TSL2591_ADDR, gs_uReg, tsl2591SuccessCallback, tsl2591ErrorCallback);
                
                gs_uRegisterPointer = TSL2591_REGISTER_CONTROL;
                gs_sTSL2591Object.m_eState = eTSL2591_CONFIGURING;
            }
            break;
        }
        case eTSL2591_CONFIGURING:
        {
            // Wait success or error handler
            break;
        }
        case eTSL2591_ALS_IDLE:
        {
            
            // Wait request to start ALS conversion
            break;
        }
        case eTSL2591_REQUEST_RECEIVED:
        {
            if (i2cIsBusIdle())
            {
                gs_uWriteBuffer = (TSL2591_ENABLE_POWERON | TSL2591_ENABLE_AEN);
                gs_uReg = (TSL2591_COMMAND_BIT | TSL2591_REGISTER_ENABLE);
                
                i2cWrite(&gs_uWriteBuffer, 1, TSL2591_ADDR, gs_uReg, tsl2591SuccessCallback, tsl2591ErrorCallback);
                
                gs_uRegisterPointer = TSL2591_REGISTER_ENABLE;
                gs_sTSL2591Object.m_eState = eTSL2591_ENABLING; // Change state if i2c request is accepted
            }
            break;
        }
        case eTSL2591_ENABLING:
        {
            // Wait success or error handler
            break;
        }
        case eTSL2591_ENABLED:
        {
            if(gs_uRegisterPointer !=  TSL2591_REGISTER_DEVICE_STATUS)
            {
                tsl2591SetRegisterPointer(TSL2591_REGISTER_DEVICE_STATUS);
                break;
            }
            // Start a read to AVALID bit
            if (i2cIsBusIdle())
            {
                gs_uReg = (TSL2591_COMMAND_BIT | TSL2591_REGISTER_DEVICE_STATUS);
                
                i2cRead(gs_uReadBuffer, 1, TSL2591_ADDR, gs_uReg, tsl2591SuccessCallback, tsl2591ErrorCallback);
                gs_sTSL2591Object.m_eState = eTSL2591_POLLING_AVALID;   // Change state if i2c request is accepted

            }
            break;
        }
        case eTSL2591_POLLING_AVALID:
        {
            // Wait success or error handler
            break;
        }
        case eTSL2591_DATA_READY:
        {
            if(gs_uRegisterPointer !=  TSL2591_REGISTER_CHAN0_LOW)
            {
                tsl2591SetRegisterPointer(TSL2591_REGISTER_CHAN0_LOW);
                break;
            }
            // Start a read to get raw data
            if (i2cIsBusIdle())
            {
                gs_uReg = (TSL2591_COMMAND_BIT | TSL2591_REGISTER_CHAN0_LOW);
                i2cRead(gs_uReadBuffer, 4, TSL2591_ADDR, gs_uReg, tsl2591SuccessCallback, tsl2591ErrorCallback);
                
                gs_sTSL2591Object.m_eState = eTSL2591_READING_RAW;   // Change state if i2c request is accepted
                
            }
            break;
        }
        case eTSL2591_READING_RAW:
        {
            // Wait success or error handler
            break;
        }
        case eTSL2591_DATA_RECEIVED:
        {
            gs_sTSL2591Object.m_uRawFull32 = (uint32_t) gs_uReadBuffer[0] + ((uint32_t) gs_uReadBuffer[1] << 8) + ((uint32_t) gs_uReadBuffer[2] << 16) + ((uint32_t) gs_uReadBuffer[3] << 24);
            gs_sTSL2591Object.m_uRawFull16 = (uint16_t)gs_uReadBuffer[0] + ((uint16_t) gs_uReadBuffer[1] << 8);
            gs_sTSL2591Object.m_uRawIr16 = (uint16_t) gs_uReadBuffer[2] + ((uint16_t) gs_uReadBuffer[3] << 8);
            
            tsl2591ReturnValues();  // Return values and call user callback
            
            gs_sTSL2591Object.m_eState = eTLS2591_DISABLE_ALS;
            break;
        }
        case eTLS2591_DISABLE_ALS:
        {
            // Start a write to disable ALS to reset the AVALID flag in the status register
            if (i2cIsBusIdle())
            {
                gs_uReg = (TSL2591_COMMAND_BIT | TSL2591_REGISTER_ENABLE);
                gs_uWriteBuffer = (TSL2591_ENABLE_POWERON);
                i2cWrite(&gs_uWriteBuffer, 1, TSL2591_ADDR, gs_uReg, tsl2591SuccessCallback, tsl2591ErrorCallback);
                
                gs_sTSL2591Object.m_eState = eTSL2591_DISABLING; // Change state if i2c request is accepted
                gs_uRegisterPointer = TSL2591_REGISTER_ENABLE;
            }
            break;
        }
        case eTSL2591_DISABLING:
        {
            // Wait success or error handler
            break;
        }
        default:
        {
            break;
        }
    }
    
}

bool tsl2591SenseLight(uint16_t* pFull, uint16_t* pIr, float* pLux, void (*senseCompleteHandler)())
{
    bool bSuccess = false;
    
    if (gs_sTSL2591Object.m_eState == eTSL2591_ALS_IDLE)
    {
        gs_sTSL2591Object.m_Callback = senseCompleteHandler;
        gs_sTSL2591Object.m_pFullOutputVal = pFull;
        gs_sTSL2591Object.m_pIrOutputVal = pIr;
        gs_sTSL2591Object.m_pLuxOuputVal = pLux;
        
        gs_sTSL2591Object.m_eState = eTSL2591_REQUEST_RECEIVED;
        
        bSuccess = true;
    }
    
    return bSuccess;
}

bool tsl2591IsStarted(void)
{
    return ((gs_sTSL2591Object.m_eState != eTSL2591_NOT_CONFIGURED) && (gs_sTSL2591Object.m_eState != eTSL2591_CONFIGURING));
}

bool tsl2591IsReadyForRequest(void)
{
    return (gs_sTSL2591Object.m_eState == eTSL2591_ALS_IDLE);
}

void tsl2591ReturnValues(void)
{
    // Calculate Lux
    tsl2591CalculateLux();  
    
    // Copy results in the output variables specified by the user
    *gs_sTSL2591Object.m_pFullOutputVal = gs_sTSL2591Object.m_uRawFull16;
    *gs_sTSL2591Object.m_pIrOutputVal = gs_sTSL2591Object.m_uRawIr16;
    
    if(*gs_sTSL2591Object.m_pLuxOuputVal == 0)
    {
        *gs_sTSL2591Object.m_pLuxOuputVal = gs_sTSL2591Object.m_fLux;
    }
    else
    {
        *gs_sTSL2591Object.m_pLuxOuputVal = *gs_sTSL2591Object.m_pLuxOuputVal*0.8 + gs_sTSL2591Object.m_fLux*0.2;
    }
    
    // Execute callback if one exist
    if (gs_sTSL2591Object.m_Callback != NULL)
    {
        gs_sTSL2591Object.m_Callback();
    }
}

void tsl2591CalculateLux(void)
{
    float atime, again, cpl, lux;
    uint16_t ch0 = gs_sTSL2591Object.m_uRawFull16;
    uint16_t ch1 = gs_sTSL2591Object.m_uRawIr16;
    
    if ((ch0 != 0) && (ch1 != 0))
    {
        switch (gs_sTSL2591Object.m_eTime) 
        {
        case TSL2591_INTEGRATIONTIME_100MS:
          atime = 100.0F;
          break;
        case TSL2591_INTEGRATIONTIME_200MS:
          atime = 200.0F;
          break;
        case TSL2591_INTEGRATIONTIME_300MS:
          atime = 300.0F;
          break;
        case TSL2591_INTEGRATIONTIME_400MS:
          atime = 400.0F;
          break;
        case TSL2591_INTEGRATIONTIME_500MS:
          atime = 500.0F;
          break;
        case TSL2591_INTEGRATIONTIME_600MS:
          atime = 600.0F;
          break;
        default: // 100ms
          atime = 100.0F;
          break;
        }

      switch (gs_sTSL2591Object.m_eGain) 
      {
            case TSL2591_GAIN_LOW:
              again = 1.0F;
              break;
            case TSL2591_GAIN_MED:
              again = 25.0F;
              break;
            case TSL2591_GAIN_HIGH:
              again = 428.0F;
              break;
            case TSL2591_GAIN_MAX:
              again = 9876.0F;
              break;
            default:
              again = 1.0F;
              break;
      }

      // cpl = (ATIME * AGAIN) / DF
      cpl = (atime * again) / TSL2591_LUX_DF;

      // Alternate lux calculation 1
      // See: https://github.com/adafruit/Adafruit_TSL2591_Library/issues/14
      lux = (((float)ch0 - (float)ch1)) * (1.0F - ((float)ch1 / (float)ch0)) / cpl;

      gs_sTSL2591Object.m_fLux = lux;
    }
}

void tsl2591SuccessCallback(void)
{
    switch (gs_sTSL2591Object.m_eState)
    {
        case eTSL2591_NOT_CONFIGURED:
        {
            // No transmission in this state
            break;
        }
        case eTSL2591_CONFIGURING:
        {
            gs_sTSL2591Object.m_eState = eTSL2591_ALS_IDLE;
            break;
        }
        case eTSL2591_ALS_IDLE:
        {
            // No transmission in this state
            break;
        }
        case eTSL2591_REQUEST_RECEIVED:
        {
            // No transmission in this state
            break;
        }
        case eTSL2591_ENABLING:
        {
            gs_sTSL2591Object.m_eState = eTSL2591_ENABLED;
            break;
        }
        case eTSL2591_ENABLED:
        {
            // No transmission in this state
            break;
        }
        case eTSL2591_POLLING_AVALID:
        {
            if ((gs_uReadBuffer[0] & 0x01) == 0x01) // AVALID bit = 1
            {
                gs_sTSL2591Object.m_eState = eTSL2591_DATA_READY;
            }
            else
            {
                gs_sTSL2591Object.m_eState = eTSL2591_ENABLED;
            }
            break;
        }
        case eTSL2591_DATA_READY:
        {
            // No transmission in this state
            break;
        }
        case eTSL2591_READING_RAW:
        {
            gs_sTSL2591Object.m_eState = eTSL2591_DATA_RECEIVED;
            break;
        }
        case eTSL2591_DATA_RECEIVED:
        {
           // No transmission in this state
            break;
        }
        case eTLS2591_DISABLE_ALS:
        {
            // No transmission in this state
            break;
        }
        case eTSL2591_DISABLING:
        {
            gs_sTSL2591Object.m_eState = eTSL2591_ALS_IDLE;
            break;
        }
        default:
        {
            break;
        }
    }
}
void tsl2591ErrorCallback(void)
{
    switch (gs_sTSL2591Object.m_eState)
    {
        case eTSL2591_NOT_CONFIGURED:
        {
            // No transmission in this state
            break;
        }
        case eTSL2591_CONFIGURING:
        {
            gs_sTSL2591Object.m_eState = eTSL2591_NOT_CONFIGURED;
            break;
        }
        case eTSL2591_ALS_IDLE:
        {
            // No transmission in this state
            break;
        }
        case eTSL2591_REQUEST_RECEIVED:
        {
            // No transmission in this state
            break;
        }
        case eTSL2591_ENABLING:
        {
            gs_sTSL2591Object.m_eState = eTSL2591_REQUEST_RECEIVED;
            break;
        }
        case eTSL2591_ENABLED:
        {
            // No transmission in this state
            break;
        }
        case eTSL2591_POLLING_AVALID:
        {
            gs_sTSL2591Object.m_eState = eTSL2591_ENABLED;
            break;
        }
        case eTSL2591_DATA_READY:
        {
            // No transmission in this state
            break;
        }
        case eTSL2591_READING_RAW:
        {
            gs_sTSL2591Object.m_eState = eTSL2591_DATA_READY;
            break;
        }
        case eTSL2591_DATA_RECEIVED:
        {
            // No transmission in this state
            break;
        }
        case eTLS2591_DISABLE_ALS:
        {
            // No transmission in this state
            break;
        }
        case eTSL2591_DISABLING:
        {
            gs_sTSL2591Object.m_eState = eTLS2591_DISABLE_ALS;
            break;
        }
        default:
        {
            break;
        }
    }
}

uint8_t TSLgetIntegrationTime(void)
{
    return gs_sTSL2591Object.m_eTime;
}

void TSLset_parameters(uint8_t gain, uint8_t integ)
{
    gs_sTSL2591Object.m_eTime = (tsl2591IntegrationTime_t)integ;
    gs_sTSL2591Object.m_eGain = (tsl2591Gain_t) (gain << 4);
    
    gs_sTSL2591Object.m_eState = eTSL2591_NOT_CONFIGURED;
    
}

void tsl2591SetRegisterPointer(uint8_t reg)
{
    if (i2cIsBusIdle())
    {

        gs_uWriteBuffer = 0;
        gs_uReg = (TSL2591_COMMAND_BIT | reg);
                
        i2cWrite(&gs_uWriteBuffer, 0, TSL2591_ADDR, gs_uReg, tsl2591RegisterPointerSuccess, NULL);

    } 
}

void tsl2591RegisterPointerSuccess(void)
{
    switch (gs_sTSL2591Object.m_eState)
    {
        case eTSL2591_NOT_CONFIGURED:
        {
            break;
        }
        case eTSL2591_CONFIGURING:
        {
            break;
        }
        case eTSL2591_ALS_IDLE:
        {
            break;
        }
        case eTSL2591_REQUEST_RECEIVED:
        {
            // No transmission in this state
            break;
        }
        case eTSL2591_ENABLING:
        {
            break;
        }
        case eTSL2591_ENABLED:
        {
            gs_uRegisterPointer = TSL2591_REGISTER_DEVICE_STATUS;
            break;
        }
        case eTSL2591_POLLING_AVALID:
        {
            break;
        }
        case eTSL2591_DATA_READY:
        {
            gs_uRegisterPointer = TSL2591_REGISTER_CHAN0_LOW;
            break;
        }
        case eTSL2591_READING_RAW:
        {
            break;
        }
        case eTSL2591_DATA_RECEIVED:
        {
            break;
        }
        case eTLS2591_DISABLE_ALS:
        {
            break;
        }
        case eTSL2591_DISABLING:
        {
            break;
        }
        default:
        {
            break;
        }
    } 
}