/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : TemperatureManager.c
  * @brief          : Acquire ADC value, filter and scale to temperature in F
  ******************************************************************************
  * @attention
  *
  * NOTICE:  All information contained herein is, and remains
  * the property of SBI.  The intellectual and technical concepts contained
  * herein are proprietary to SBI may be covered by Canadian, US and Foreign Patents,
  * patents in process, and are protected by trade secret or copyright law.
  * Dissemination of this information or reproduction of this material
  * is strictly forbidden unless prior written permission is obtained
  * from SBI.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
//#include "stm32f1xx_hal_i2c.h"

/* Private includes ----------------------------------------------------------*/
#include "cmsis_os.h"
#include "stm32f1xx_hal.h"
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include "main.h" //@@@@ for the LED toggle to be removed
#include "algo.h"
#include <stdio.h>
#include "DebugPort.h"
#include "MotorManager.h"
#include "TemperatureManager.h"

/* Private typedef -----------------------------------------------------------*/


/* Private define ------------------------------------------------------------*/
#define ADC_ADDRESS_7BIT (0x68 << 1)
#define IsDataNew(x) ((x & 0x80)==0)

/* Private macro -------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/
static osSemaphoreId I2CSemaphoreHandle;
static int Tboard;

/* Private function ---------------------------------------------------------*/
float uVtoDegreeCTypeK(float uVdata,float Tref);
float VtoDegreeCRtd(float Vdata);

enum ADC_Channel
{
	BaffleThermocouple = 0,
	FrontThermocouple,
	PlenumRtd,
	TempSense_board, // Used to determine cold junction temp
	NUMBER_OF_ADC_CH
};

void TemperatureManager(void const * argument)
{
  /* USER CODE BEGIN TemperatureManager */
	osSemaphoreDef(I2CSemaphoreHandle);
    I2CSemaphoreHandle = osSemaphoreCreate(osSemaphore(I2CSemaphoreHandle), 1);
    osSemaphoreWait(I2CSemaphoreHandle,1); //decrement semaphore value for the lack of way to create a semaphore with a count of 0.

    uint8_t ADCConfigByte[NUMBER_OF_ADC_CH] = {0x9F,0xBF,0xDC,0xFC}; // Channel 3 is for RTD,Gain=1 //channel 4 is for the pressure sensor, Gain =1
    int32_t i32tempReading=0;
    int i =0;
    uint8_t adcData[4];

    float AdcArray[NUMBER_OF_ADC_CH];
    float TemperatureCelsius[NUMBER_OF_ADC_CH];
    float TemperatureFarenheit[NUMBER_OF_ADC_CH];
	float tColdJunction;
	float temp1;
    uint32_t PreviousWakeTime = osKernelSysTick(); //must be nitialized before first use

    bool DataReady;

	temp1 = 0.800;
	tColdJunction = (temp1-0.500)/.010;

    /* Infinite loop */
    for(;;)
    {
    	osDelayUntil(&PreviousWakeTime,5000);
    	//HAL_GPIO_TogglePin(USB_LED_GPIO_Port,USB_LED_Pin);

    	//coldjunction temperature
		//temp1 = uCAdcData[1]*3.3/4096;  //Vout=TC x TA + VoC where TC = 10mV/C V0C->500mV


		for (i=TempSense_board; i >= 0; i--)
		{

			HAL_I2C_Master_Transmit_IT(&hi2c1, ADC_ADDRESS_7BIT,&ADCConfigByte[i],1);
			//osSemaphoreWait(I2CSemaphoreHandle,osWaitForever); //wait forever @@@@ to restart and I2C transaction in case of hang up sys
			if(osErrorOS == osSemaphoreWait(I2CSemaphoreHandle,1000)) //wait 500ms for an answer or retry
			{
				//clearly something is wrong Abort the transmission
				//HAL_GPIO_WritePin(STATUS_LED0_GPIO_Port,STATUS_LED0_Pin,RESET);
				HAL_I2C_Master_Abort_IT(&hi2c1,ADC_ADDRESS_7BIT);
				HAL_I2C_DeInit(&hi2c1);
				osDelay(100);
				MX_I2C1_Init();
				osDelay(100);
			}
			else
			{
				//do something in the callback
				HAL_GPIO_WritePin(STATUS_LED1_GPIO_Port,STATUS_LED1_Pin,RESET);
				do{
					DataReady = false;
					osDelay(300); //wait to give the chance to the ADC to complete the conversion 1/3.75 = 266ms
					HAL_I2C_Master_Receive_IT(&hi2c1, ADC_ADDRESS_7BIT,adcData,4);
					//osSemaphoreWait(I2CSemaphoreHandle,osWaitForever); //wait forever @@@@ to restart and I2C transaction in case of hang up sys
					if(osErrorOS == osSemaphoreWait(I2CSemaphoreHandle,500)) //wait 500ms for an answer or retry
					{
						continue;
					}
					else
					{
						DataReady = (IsDataNew(adcData[3]));
					}

				}while (!DataReady);
			}
			i32tempReading = 0;
			i32tempReading = (adcData[0] << 30) + (adcData[1] << 22)  + (adcData[2] << 14); // justify the result for 32bit storage
			if (i32tempReading < 0){
				i32tempReading = -i32tempReading;
			}
			i32tempReading = (i32tempReading) >> 14;

			//AdcArray[i] = ((float)(abs(i32tempReading))*15.625)/8; //15.625uV par bit  gain = 8
			switch(i)
			{
				case FrontThermocouple:
				case BaffleThermocouple:
					AdcArray[i] = ((float)(i32tempReading)*15.625)/8; //15.625uV par bit  gain = 8
					TemperatureCelsius[i] = uVtoDegreeCTypeK(AdcArray[i], tColdJunction); //6.7//26.1 //board is self heating to 7.3 above ambient
					break;
				case PlenumRtd:
					AdcArray[i] = (float)(i32tempReading*15.625e-6);
					TemperatureCelsius[i] = VtoDegreeCRtd(AdcArray[i]);
					break;
				case TempSense_board:
					AdcArray[i] = (float)(i32tempReading*15.625e-6);
					TemperatureCelsius[i] = (AdcArray[i]-0.500)/.010;
					tColdJunction = TemperatureCelsius[i];
					break;
				default:
					break;
			}

			TemperatureFarenheit[i] = TemperatureCelsius[i]*9/5+32;

		}

		Tboard = (int)(TemperatureFarenheit[TempSense_board]);
		Algo_setBaffleTemp((int)(TemperatureFarenheit[BaffleThermocouple]*10));
		Algo_setFrontTemp((int)(TemperatureFarenheit[FrontThermocouple]*10));
		Algo_setPlenumTemp((int)(TemperatureFarenheit[PlenumRtd]*10));

  	}
  /* USER CODE END TemperatureManager */
}

int get_BoardTemp(void)
{
	return Tboard;
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	osSemaphoreRelease(I2CSemaphoreHandle);
}
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	osSemaphoreRelease(I2CSemaphoreHandle);
}
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
	uint32_t errorcode = hi2c->ErrorCode;
	UNUSED(errorcode);
}
void HAL_I2C_AbortCpltCallback(I2C_HandleTypeDef *hi2c)
{
	uint32_t errorcode = hi2c->ErrorCode;
	UNUSED(errorcode);
}
//https://srdata.nist.gov/its90/type_k/kcoefficients_inverse.html

//0C to 500C 0.04 deg C to -0.05dec   0mV to 20.644
#define T_COEF_D0_0_500 0
#define T_COEF_D1_0_500 2.508355E1
#define T_COEF_D2_0_500 7.860106E-2
#define T_COEF_D3_0_500 -2.503131E-1
#define T_COEF_D4_0_500 8.315270E-2
#define T_COEF_D5_0_500 -1.228034E-2
#define T_COEF_D6_0_500 9.804036E-4
#define T_COEF_D7_0_500 -4.413030E-5
#define T_COEF_D8_0_500 1.057734E-6
#define T_COEF_D9_0_500 -1.052755E-8

#define T_COEF_D0_500_1372 -1.318058E2
#define T_COEF_D1_500_1372 4.830222E1
#define T_COEF_D2_500_1372 -1.6466031E0
#define T_COEF_D3_500_1372 5.464731E-2
#define T_COEF_D4_500_1372 -9.650715E-4
#define T_COEF_D5_500_1372 8.802193E-6
#define T_COEF_D6_500_1372 -3.110810E-8

//from temperature to voltage
#define T_COEF_C0 -1.7600413686E-2
#define T_COEF_C1 3.8921204975E-2
#define T_COEF_C2 1.8558770032E-5
#define T_COEF_C3 -9.9457592874E-8
#define T_COEF_C4 3.1840945719E-10
#define T_COEF_C5 -5.6072844889E-13
#define T_COEF_C6 5.6075059059E-16
#define T_COEF_C7 -3.2020720003E-19
#define T_COEF_C8 9.7151147152E-23
#define T_COEF_C9 -1.2104721275E-26

#define T_COEF_A0 1.185976E-1
#define T_COEF_A1 -1.183432E-4
#define T_COEF_A2 1.269686E2

float uVtoDegreeCTypeK(float uVdata,float Tref)
{
    double Vref = T_COEF_C0 + T_COEF_C1*Tref + T_COEF_C2*pow(Tref,2) + T_COEF_C3*pow(Tref,3) + T_COEF_C4*pow(Tref,4) + T_COEF_C5*pow(Tref,5) + T_COEF_C6*pow(Tref,6) + T_COEF_C7*pow(Tref,7) + T_COEF_C8*pow(Tref,8) + T_COEF_C9*pow(Tref,9)+T_COEF_A0*pow(2.718281828,T_COEF_A1*(Tref-T_COEF_A2)*(Tref-T_COEF_A2));
    double Vmeas = uVdata/1000; //value need to be in mV
    double totalV = Vmeas + Vref;

    double t90;
    if(totalV < 20.644)
    {
     t90 = T_COEF_D0_0_500 + T_COEF_D1_0_500 *totalV + T_COEF_D2_0_500 *pow(totalV,2) + T_COEF_D3_0_500 *pow(totalV,3) + T_COEF_D4_0_500 *pow(totalV,4) + T_COEF_D5_0_500 *pow(totalV,5) + T_COEF_D6_0_500 *pow(totalV,6) + T_COEF_D7_0_500 *pow(totalV,7) + T_COEF_D8_0_500 *pow(totalV,8) + T_COEF_D9_0_500 *pow(totalV,9);
    }
    else
    {
	    t90 = T_COEF_D0_500_1372 + T_COEF_D1_500_1372 *totalV + T_COEF_D2_500_1372 *pow(totalV,2) + T_COEF_D3_500_1372 *pow(totalV,3) + T_COEF_D4_500_1372 *pow(totalV,4) + T_COEF_D5_500_1372 *pow(totalV,5) + T_COEF_D6_500_1372 *pow(totalV,6);
    }

    return (float)t90;
}
float VtoDegreeCRtd(float Vdata)
{
	//using a y = 366.02x^2 -942.3x +561.55 where x is the ADC voltage and y is the temperature in C
	return (Vdata*Vdata)*366.02 - 942.3*Vdata + 561.55;
}


