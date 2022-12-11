/*
Solutions Novika.
Copyright 2021
===========================================================================

Filename :      I2CEngine.h

Author(s):      Clement Soucy, CPI # 6027411

Public prefix : i2c

Project # : C-1333

Product: Mesure d'émissions polluantes

Creation date:  2021/06/14

Description:    Implementation of I2C engine to work on top of MCC I2C master driver.

===========================================================================

Modifications
-------------

By    |   Date     | Version | Description
------+------------+---------+---------------------------------------------
CS    | 2021/06/14 | -       | Creation
===========================================================================
*/ 
#ifndef I2CENGINE_H
#define	I2CENGINE_H

#include <xc.h>
#include <stdbool.h>

void i2cInitialize(void);

bool i2cIsBusIdle(void);

void i2cWrite(uint8_t* buffer, uint8_t size, uint8_t deviceAddress, uint8_t registerAdress, void (*successCallback)(), void (*errorCallback)());

void i2cRead(uint8_t* buffer, uint8_t size, uint8_t deviceAddress, uint8_t registerAdress, void (*successCallback)(), void (*errorCallback)());

void i2cProcess(void);

void I2C_ReadNByteRegister(uint8_t reg, uint8_t* data, uint8_t len);

void I2C_WriteNByteRegister(uint8_t reg, uint8_t* data, uint8_t len);

#endif	/* I2CENGINE_H */

