/**
  Generated Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This is the main file generated using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  Description:
    This header file provides implementations for driver APIs for all modules selected in the GUI.
    Generation Information :
        Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.81.7
        Device            :  PIC18F16Q40
        Driver Version    :  2.00
*/
#include "ParameterFile.h"
#include "mcc_generated_files/mcc.h"
#include "DebugInterface.h"
#include "I2CEngine.h"
#include "TSL2591.h"
#include "GPIO.h"
#include "DS1775.h"
#include "MeasureParticles.h"
#include "ControlBridge.h"
#include "ChillerManager.h" 


/*
                         Main application
 */

#pragma warning disable 520,1510,2053

void main(void)
{
    // Initialize the device
    SYSTEM_Initialize();
    ParameterInit();
    measureParticlesInitialize();
    GpioInitialize();
    DebugInitialize();
    ds1775Initialize();
    tsl2591Initialize();
    i2cInitialize();
    ControlBridgeInitialize();
    ChillerInit();


    // If using interrupts in PIC18 High/Low Priority Mode you need to enable the Global High and Low Interrupts
    // If using interrupts in PIC Mid-Range Compatibility Mode you need to enable the Global Interrupts
    // Use the following macros to:

    // Enable the Global Interrupts
    INTERRUPT_GlobalInterruptEnable();

    // Disable the Global Interrupts
    //INTERRUPT_GlobalInterruptDisable();
    
        
    while (1)
    {
        tsl2591Process();
        
        DebugProcess();
        ds1775Process();
        i2cProcess();
        ControlBridgeProcess();
        
        measureParticlesProcess();
        
        ChillerProcess();
        // Add your application code
    }
}
/**
 End of File
*/