/**
  Generated Interrupt Manager Source File

  @Company:
    Microchip Technology Inc.

  @File Name:
    interrupt_manager.c

  @Summary:
    This is the Interrupt Manager file generated using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  @Description:
    This header file provides implementations for global interrupt handling.
    For individual peripheral handlers please see the peripheral driver for
    all modules selected in the GUI.
    Generation Information :
        Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.81.7
        Device            :  PIC18F16Q40
        Driver Version    :  2.04
    The generated drivers are tested against the following:
        Compiler          :  XC8 2.31 and above or later
        MPLAB 	          :  MPLAB X 5.45
*/

/*
    (c) 2018 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
    SOFTWARE.
*/

#include "interrupt_manager.h"
#include "mcc.h"

void  INTERRUPT_Initialize (void)
{
    // Disable Interrupt Priority Vectors (16CXXX Compatibility Mode)
    INTCON0bits.IPEN = 0;
}

void __interrupt() INTERRUPT_InterruptManager (void)
{
    // interrupt handler
    if(PIE8bits.U2IE == 1 && PIR8bits.U2IF == 1)
    {
        UART2_UARTInterruptHandler();
    }
    else if(PIE8bits.U2TXIE == 1 && PIR8bits.U2TXIF == 1)
    {
        UART2_TxInterruptHandler();
    }
    else if(PIE8bits.U2RXIE == 1 && PIR8bits.U2RXIF == 1)
    {
        UART2_RxInterruptHandler();
    }
    else if(PIE4bits.U1TXIE == 1 && PIR4bits.U1TXIF == 1)
    {
        UART1_TxInterruptHandler();
    }
    else if(PIE4bits.U1RXIE == 1 && PIR4bits.U1RXIF == 1)
    {
        UART1_RxInterruptHandler();
    }
    else if(PIE3bits.TMR0IE == 1 && PIR3bits.TMR0IF == 1)
    {
        TMR0_ISR();
    }
    else if(PIE7bits.I2C1EIE == 1 && PIR7bits.I2C1EIF == 1)
    {
        I2C1_InterruptHandler();
    }
    else if(PIE7bits.I2C1RXIE == 1 && PIR7bits.I2C1RXIF == 1)
    {
        I2C1_InterruptHandler();
    }
    else if(PIE7bits.I2C1IE == 1 && PIR7bits.I2C1IF == 1)
    {
        I2C1_InterruptHandler();
    }
    else if(PIE7bits.I2C1TXIE == 1 && PIR7bits.I2C1TXIF == 1)
    {
        I2C1_InterruptHandler();
    }
    else if(PIE3bits.TMR1IE == 1 && PIR3bits.TMR1IF == 1)
    {
        TMR1_ISR();
    }
    else
    {
        //Unhandled Interrupt
    }
}
/**
 End of File
*/
