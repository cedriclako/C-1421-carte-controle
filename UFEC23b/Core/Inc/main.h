/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2023 STMicroelectronics.
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
#include <WhiteBox.h>
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
void MX_I2C1_Init(void);
void MX_USART3_UART_Init(void);
void MX_USART2_UART_Init(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Step2_LowCurrent_Pin GPIO_PIN_13
#define Step2_LowCurrent_GPIO_Port GPIOC
#define Limit_switch1_Pin GPIO_PIN_0
#define Limit_switch1_GPIO_Port GPIOC
#define Limit_switch2_Pin GPIO_PIN_1
#define Limit_switch2_GPIO_Port GPIOC
#define Step3_RESET_Pin GPIO_PIN_2
#define Step3_RESET_GPIO_Port GPIOC
#define AFK_Speed1_Pin GPIO_PIN_3
#define AFK_Speed1_GPIO_Port GPIOC
#define FAN_Zero_crossing_Pin GPIO_PIN_1
#define FAN_Zero_crossing_GPIO_Port GPIOA
#define FAN_Zero_crossing_EXTI_IRQn EXTI1_IRQn
#define Limit_switch3_Pin GPIO_PIN_4
#define Limit_switch3_GPIO_Port GPIOA
#define Step3_DIR_Pin GPIO_PIN_5
#define Step3_DIR_GPIO_Port GPIOA
#define AFK_SpeedPWM_Pin GPIO_PIN_6
#define AFK_SpeedPWM_GPIO_Port GPIOA
#define Step3_LowCurrent_Pin GPIO_PIN_7
#define Step3_LowCurrent_GPIO_Port GPIOA
#define uc_Stepper_Sleep_Pin GPIO_PIN_4
#define uc_Stepper_Sleep_GPIO_Port GPIOC
#define Step3_ENABLE_Pin GPIO_PIN_5
#define Step3_ENABLE_GPIO_Port GPIOC
#define Limit_switch_Door_Pin GPIO_PIN_0
#define Limit_switch_Door_GPIO_Port GPIOB
#define Step3_STEP_Pin GPIO_PIN_1
#define Step3_STEP_GPIO_Port GPIOB
#define BOOT1_input_Pin GPIO_PIN_2
#define BOOT1_input_GPIO_Port GPIOB
#define Thermostat_Input_Pin GPIO_PIN_12
#define Thermostat_Input_GPIO_Port GPIOB
#define Safety_Out_Pin GPIO_PIN_13
#define Safety_Out_GPIO_Port GPIOB
#define Interlock_Input_Pin GPIO_PIN_14
#define Interlock_Input_GPIO_Port GPIOB
#define Reset_Particles_Sensor_Pin GPIO_PIN_15
#define Reset_Particles_Sensor_GPIO_Port GPIOB
#define SPEED2_COIL_Pin GPIO_PIN_6
#define SPEED2_COIL_GPIO_Port GPIOC
#define FAN_SPEED3_Pin GPIO_PIN_7
#define FAN_SPEED3_GPIO_Port GPIOC
#define Step2_DIR_Pin GPIO_PIN_8
#define Step2_DIR_GPIO_Port GPIOC
#define Step1_LowCurrent_Pin GPIO_PIN_9
#define Step1_LowCurrent_GPIO_Port GPIOC
#define Button_LED_Pin GPIO_PIN_8
#define Button_LED_GPIO_Port GPIOA
#define Safety_ON_Pin GPIO_PIN_15
#define Safety_ON_GPIO_Port GPIOA
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
#define Button_Input_Pin GPIO_PIN_8
#define Button_Input_GPIO_Port GPIOB
#define USB_Fault_Pin GPIO_PIN_9
#define USB_Fault_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define NOVIKA_SETUP (0)
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
