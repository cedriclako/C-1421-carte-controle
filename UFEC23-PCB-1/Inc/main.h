/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void MX_I2C1_Init(void);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define step2_LowCurrent_Pin GPIO_PIN_13
#define step2_LowCurrent_GPIO_Port GPIOC
#define LimitSwith1_Pin GPIO_PIN_0
#define LimitSwith1_GPIO_Port GPIOC
#define LimitSwitch2_Pin GPIO_PIN_1
#define LimitSwitch2_GPIO_Port GPIOC
#define Remote_BTN_Pin GPIO_PIN_2
#define Remote_BTN_GPIO_Port GPIOC
#define Remote_led_Pin GPIO_PIN_3
#define Remote_led_GPIO_Port GPIOC
#define SPEED3_COIL_Pin GPIO_PIN_1
#define SPEED3_COIL_GPIO_Port GPIOA
#define SPEED2_COIL_Pin GPIO_PIN_2
#define SPEED2_COIL_GPIO_Port GPIOA
#define SPEED1_COIL_Pin GPIO_PIN_3
#define SPEED1_COIL_GPIO_Port GPIOA
#define Rev_bit1_Pin GPIO_PIN_4
#define Rev_bit1_GPIO_Port GPIOA
#define Rev_bit0_Pin GPIO_PIN_5
#define Rev_bit0_GPIO_Port GPIOA
#define uc_Stepper_Sleep_Pin GPIO_PIN_4
#define uc_Stepper_Sleep_GPIO_Port GPIOC
#define STATUS_LED0_Pin GPIO_PIN_5
#define STATUS_LED0_GPIO_Port GPIOC
#define Button_LED_Pin GPIO_PIN_0
#define Button_LED_GPIO_Port GPIOB
#define USB_LED_Pin GPIO_PIN_1
#define USB_LED_GPIO_Port GPIOB
#define Test_Pin_Pin GPIO_PIN_2
#define Test_Pin_GPIO_Port GPIOB
#define Thermostat_input_Pin GPIO_PIN_12
#define Thermostat_input_GPIO_Port GPIOB
#define Safety_ON_Pin GPIO_PIN_13
#define Safety_ON_GPIO_Port GPIOB
#define Interlock_input_Pin GPIO_PIN_14
#define Interlock_input_GPIO_Port GPIOB
#define STATUS_LED1_Pin GPIO_PIN_6
#define STATUS_LED1_GPIO_Port GPIOC
#define STATUS_LED2_Pin GPIO_PIN_7
#define STATUS_LED2_GPIO_Port GPIOC
#define Step2_DIR_Pin GPIO_PIN_8
#define Step2_DIR_GPIO_Port GPIOC
#define Step1_LowCurrent_Pin GPIO_PIN_9
#define Step1_LowCurrent_GPIO_Port GPIOC
#define Model_bit0_Pin GPIO_PIN_8
#define Model_bit0_GPIO_Port GPIOA
#define Model_bit1_Pin GPIO_PIN_9
#define Model_bit1_GPIO_Port GPIOA
#define Model_bit2_Pin GPIO_PIN_10
#define Model_bit2_GPIO_Port GPIOA
#define Step2_STEP_Pin GPIO_PIN_10
#define Step2_STEP_GPIO_Port GPIOC
#define Step2_RESET_Pin GPIO_PIN_11
#define Step2_RESET_GPIO_Port GPIOC
#define Step2_ENABLE_Pin GPIO_PIN_12
#define Step2_ENABLE_GPIO_Port GPIOC
#define Step1_DIR_Pin GPIO_PIN_2
#define Step1_DIR_GPIO_Port GPIOD
#define Step1_STEP_Pin GPIO_PIN_3
#define Step1_STEP_GPIO_Port GPIOB
#define Step1_RESET_Pin GPIO_PIN_4
#define Step1_RESET_GPIO_Port GPIOB
#define Step1_ENABLE_Pin GPIO_PIN_5
#define Step1_ENABLE_GPIO_Port GPIOB
#define I2C_SDA_Pin GPIO_PIN_6
#define I2C_SDA_GPIO_Port GPIOB
#define I2C_SCL_Pin GPIO_PIN_7
#define I2C_SCL_GPIO_Port GPIOB
#define Button_input_Pin GPIO_PIN_8
#define Button_input_GPIO_Port GPIOB
#define USB_Fault_Pin GPIO_PIN_9
#define USB_Fault_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

extern RTC_TimeTypeDef sTime;
extern RTC_HandleTypeDef hrtc;
extern uint32_t uCAdcData[2];
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
