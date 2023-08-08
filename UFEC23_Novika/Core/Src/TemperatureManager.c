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
#include "stm32f1xx_hal.h"
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include "Algo.h"
#include <stdio.h>
#include "TemperatureManager.h"

/* Private typedef -----------------------------------------------------------*/

enum ADC_Channel
{
	BaffleThermocouple = 0,
	ChamberThermocouple,
	PlenumRtd,
	TempSense_board, // Used to determine cold junction temp
	NUMBER_OF_ADC_CH
};

typedef enum
{
	Sending_config = 0,
	Wait_for_data_rdy,
	Send_read_req,
	Response_received
}Temp_state;

typedef struct
{
	float fTcoldJunct;
	uint8_t ADCConfigByte[NUMBER_OF_ADC_CH];

}TempObj;

/* Private define ------------------------------------------------------------*/
#define ADC_ADDRESS_7BIT (0x68 << 1)
#define IsDataNew(x) ((x & 0x80)==0)

/* Private macro -------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/
static bool b_tx_pending, b_tx_success;
static bool b_rx_pending,b_rx_success;
static Temp_state currentState;
static Temp_state nextState;

static TempObj Tobj;
extern I2C_HandleTypeDef hi2c1;

/* Private function ---------------------------------------------------------*/
float uVtoDegreeCTypeK(float uVdata,float Tref);
float VtoDegreeCRtd(float Vdata);


void Temperature_Init(void)
{
	currentState = Sending_config;
	nextState = Sending_config;
	b_tx_pending = false;
	b_tx_success = false;
	b_rx_pending = false;
	b_rx_success = false;
	Tobj.ADCConfigByte[0] = 0x9F;
	Tobj.ADCConfigByte[1] = 0xBF;
	Tobj.ADCConfigByte[2] = 0xDC;
	Tobj.ADCConfigByte[3] = 0xFC;
}



void TemperatureManager(Mobj* stove, uint32_t u32time_ms)
{
	static int8_t ch_idx = NUMBER_OF_ADC_CH - 1;
	static uint8_t adcData[4];
	static uint32_t u32conf_time;
	int32_t i32tempReading=0;
	float ftempReading = 0.0;

	switch(currentState)
	{
	case Sending_config:
		if(b_tx_success)
		{
			b_tx_success = false;
			nextState = Wait_for_data_rdy;
			u32conf_time = u32time_ms;
		}
		else if(!b_tx_pending)
		{
			HAL_I2C_Master_Transmit_IT(&hi2c1, ADC_ADDRESS_7BIT,&Tobj.ADCConfigByte[ch_idx],1);
			b_tx_pending = true;
		}
		break;
	case Wait_for_data_rdy:
		if(u32time_ms - u32conf_time > 300) //Conversion time around 266 ms
		{
			nextState = Send_read_req;
		}
		break;
	case Send_read_req:

		if(b_rx_success)
		{
			b_rx_success = false;
			if(IsDataNew(adcData[3]))
			{
				adcData[3] = 0;
				nextState = Response_received;
			}

		}
		else if(!b_rx_pending)
		{
			HAL_I2C_Master_Receive_IT(&hi2c1, ADC_ADDRESS_7BIT,adcData,4);
			b_rx_pending = true;
		}

		break;
	case Response_received:
		i32tempReading = 0;
		i32tempReading = (adcData[0] << 30) + (adcData[1] << 22)  + (adcData[2] << 14); // justify the result for 32bit storage
		if (i32tempReading < 0)
		{
			i32tempReading = -i32tempReading;
		}
		i32tempReading = (i32tempReading) >> 14;

		switch(ch_idx)
		{
			case BaffleThermocouple:
				ftempReading = ((float)(i32tempReading)*15.625)/8; //15.625uV par bit  gain = 8
				stove->fBaffleTemp = CELSIUS_TO_FAHRENHEIT(uVtoDegreeCTypeK(ftempReading, Tobj.fTcoldJunct)); //6.7//26.1 //board is self heating to 7.3 above ambient

				break;
			case ChamberThermocouple:
				ftempReading = ((float)(i32tempReading)*15.625)/8; //15.625uV par bit  gain = 8
				stove->fChamberTemp = CELSIUS_TO_FAHRENHEIT(uVtoDegreeCTypeK(ftempReading, Tobj.fTcoldJunct)); //6.7//26.1 //board is self heating to 7.3 above ambient

				break;
			case PlenumRtd:
				ftempReading = (float)(i32tempReading*15.625e-6);
				stove->fPlenumTemp = CELSIUS_TO_FAHRENHEIT(VtoDegreeCRtd(ftempReading));

				break;
			case TempSense_board:
				ftempReading = (float)(i32tempReading*15.625e-6);
				Tobj.fTcoldJunct = (ftempReading-0.500)/.010;
				break;
			default:
				break;
		}

		nextState = Sending_config;
		if(ch_idx-- < 0)
		{
			ch_idx = NUMBER_OF_ADC_CH - 1;
		}
		break;
	}

	if(nextState != currentState)
	{
		currentState = nextState;
	}

}

void Temperature_update_deltaT(Mobj *stove, uint32_t u32DeltaT_ms)
{
	static float baffle = 0;
	static float chamber = 0;

	if(baffle == 0 && chamber == 0)
	{
		baffle = stove->fBaffleTemp;
		chamber = stove->fChamberTemp;
		return;
	}

	// To avoid confusion, parameters calculated per 30 seconds => (deg F / 30 sec)
	stove->fBaffleDeltaT = 30*(stove->fBaffleTemp-baffle)/((u32DeltaT_ms)/1000);
	stove->fChamberDeltaT = 30*(stove->fChamberTemp-chamber)/((u32DeltaT_ms)/1000);

	baffle = stove->fBaffleTemp;
	chamber = stove->fChamberTemp;

}



float get_BoardTemp(void)
{
	return Tobj.fTcoldJunct ;
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	b_tx_success = true;
	b_tx_pending = false;
}
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	b_rx_success = true;
	b_rx_pending = false;
}
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
	b_tx_pending = false;
	b_tx_success = false;
	b_rx_pending = false;
	b_rx_success = false;
	uint32_t errorcode = hi2c->ErrorCode;
	UNUSED(errorcode);
}
void HAL_I2C_AbortCpltCallback(I2C_HandleTypeDef *hi2c)
{
	b_tx_pending = false;
	b_tx_success = false;
	b_rx_pending = false;
	b_rx_success = false;
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


