/*
Solutions Novika.
Copyright 2021
===========================================================================

Filename :      I2CEngine.c

Author(s):      Clement Soucy, CPI # 6027411
 *              Charles Richard, CPI # 6045522

Public prefix : i2c

Project # : C-1333, C-1421

Product: Mesure d'émissions polluantes

Creation date:  2021/06/14

Description:    Implementation of I2C engine to work on top of MCC I2C master driver.

===========================================================================

Modifications
-------------

By    |   Date     | Version | Description
------+------------+---------+---------------------------------------------
CS    | 2021/06/14 | -       | Creation
CR    | 2022/10/14 | 0.1     | Corrections
===========================================================================
*/ 

#include "I2CEngine.h"
#include "mcc_generated_files/i2c1_master.h"

//#define I2C_TIMEOUT_TICKS  500    // MCC generate a default value in i2c1_master.c
#define WRITE false
#define READ true

typedef enum
{
    eI2C_IDLE = 0,
    eI2C_PREPARE_WRITE,
    eI2C_PREPARE_READ,
    eI2C_EXECUTE_WRITE,
    eI2C_EXECUTE_READ,

    eI2C_NB_STATES,
}EI2CStates;

typedef struct
{
    size_t len;
    uint8_t *data;
}i2c1_buffer_t;

typedef void (* i2cCallback) (void);

typedef struct
{
    EI2CStates m_eState;
    i2cCallback m_SucessCallback;
    i2cCallback m_ErrorCallback;
    bool m_bCompleted;
    bool m_bSuccess;
    
}SI2CObject;

static SI2CObject gs_sI2CObject;

//void I2C_WriteNByteRegister(uint8_t reg, uint8_t* data, uint8_t len);
//void I2C_ReadNByteRegister(uint8_t reg, uint8_t* data, uint8_t len);
static i2c1_operations_t OperationCompleteHandler(void *ptr);
static i2c1_operations_t OperationErrorHandler(void *ptr);


void i2cInitialize(void)
{
    gs_sI2CObject.m_eState = eI2C_IDLE;
    gs_sI2CObject.m_SucessCallback = NULL;
    gs_sI2CObject.m_ErrorCallback = NULL;
    gs_sI2CObject.m_bCompleted = false;
    gs_sI2CObject.m_bSuccess = false;
    
    //I2C1_SetTimeout(I2C_TIMEOUT_TICKS);   // MCC generate a default value in i2c1_master.c
}

void i2cProcess(void)
{

    if (gs_sI2CObject.m_bCompleted)
    {
        if(I2C1_Close()== 0)
        {
            if (gs_sI2CObject.m_bSuccess)
            {
                if (*gs_sI2CObject.m_SucessCallback != NULL)
                {
                    (*gs_sI2CObject.m_SucessCallback)();
                }
            }
            else
            {
                if (*gs_sI2CObject.m_ErrorCallback != NULL)
                {
                    (*gs_sI2CObject.m_ErrorCallback)();
                }
            }
        gs_sI2CObject.m_eState = eI2C_IDLE;
        gs_sI2CObject.m_bCompleted = false; // reset flag to call callback only once per transfer
        }  
    }

}

bool i2cIsBusIdle(void)
{
    return (gs_sI2CObject.m_eState == eI2C_IDLE);
}

void i2cWrite(uint8_t* buffer, uint8_t size, uint8_t deviceAddress, uint8_t registerAdress, void (*successCallback)(), void (*errorCallback)())
{
    
    if (gs_sI2CObject.m_eState == eI2C_IDLE)
    {
        if (I2C1_Open(deviceAddress)== 0)
        {
            I2C1_SetDataCompleteCallback(OperationCompleteHandler,NULL);
            I2C1_SetWriteCollisionCallback(OperationErrorHandler, NULL);
            I2C1_SetAddressNackCallback(OperationErrorHandler, NULL);
            I2C1_SetDataNackCallback(OperationErrorHandler, NULL);
            I2C1_SetTimeoutCallback(OperationErrorHandler, NULL);
            
            // Succeeded to get the bus            
            gs_sI2CObject.m_eState = eI2C_EXECUTE_WRITE;
            gs_sI2CObject.m_bCompleted = false; // reset flags
            gs_sI2CObject.m_bSuccess = false;
            
            gs_sI2CObject.m_SucessCallback = successCallback;   // Load callbacks
            gs_sI2CObject.m_ErrorCallback = errorCallback;
            
            I2C_WriteNByteRegister(registerAdress, buffer, size);
        }
        else
        {

            // print error to get the bus
        }
    }
    else
    {
        // print that bus is busy
    }
    
}

void i2cRead(uint8_t* buffer, uint8_t size, uint8_t deviceAddress, uint8_t registerAdress, void (*successCallback)(), void (*errorCallback)())
{

    
    if (gs_sI2CObject.m_eState == eI2C_IDLE)
    {
        if (I2C1_Open(deviceAddress)== 0)
        {
            I2C1_SetDataCompleteCallback(OperationCompleteHandler,NULL);
            I2C1_SetWriteCollisionCallback(OperationErrorHandler, NULL);
            I2C1_SetAddressNackCallback(OperationErrorHandler, NULL);
            I2C1_SetDataNackCallback(OperationErrorHandler, NULL);
            I2C1_SetTimeoutCallback(OperationErrorHandler, NULL);
            
            // Succeeded to get the bus
            gs_sI2CObject.m_eState = eI2C_EXECUTE_READ;
            gs_sI2CObject.m_bCompleted = false; // reset flags
            gs_sI2CObject.m_bSuccess = false;
            
            gs_sI2CObject.m_SucessCallback = successCallback;   // Load callbacks
            gs_sI2CObject.m_ErrorCallback = errorCallback;
 
            I2C_ReadNByteRegister(registerAdress, buffer, size);


        }
        else
        {

            // print error to get the bus
        }
    }
    else
    {
        // print that bus is busy
    }
}

void I2C_WriteNByteRegister(uint8_t reg, uint8_t data[], uint8_t len)
{
    uint8_t bufferBlock[10];
    bufferBlock[0] = reg;
    
    for(int i = 0; i < len; i++)
    {
        bufferBlock[i+1] = data[i];
    }

    I2C1_SetBuffer(bufferBlock, len + 1);
    
    I2C1_MasterOperation(WRITE);
}

void I2C_ReadNByteRegister(uint8_t reg, uint8_t* data, uint8_t len)
{
    data[0] = reg;
    I2C1_SetBuffer(data,len);

    I2C1_MasterOperation(READ);

}

static i2c1_operations_t OperationCompleteHandler(void *ptr)
{
    gs_sI2CObject.m_bCompleted = true;
    gs_sI2CObject.m_bSuccess = true;
   
    return I2C1_STOP;
}

static i2c1_operations_t OperationErrorHandler(void *ptr)
{
    
    gs_sI2CObject.m_bCompleted = true;
    gs_sI2CObject.m_bSuccess = false;
        
    if(gs_sI2CObject.m_eState == eI2C_EXECUTE_WRITE)
    {
        return I2C1_RESTART_WRITE;
    }
    else if(gs_sI2CObject.m_eState == eI2C_EXECUTE_READ)
    {
        return I2C1_RESTART_READ;
    }
    return I2C1_STOP;
}
