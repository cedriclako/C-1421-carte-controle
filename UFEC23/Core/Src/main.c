/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "MotorManager.h"
#include "TemperatureManager.h"
#include "DebugPort.h"
#include "TestManager.h"
#include "DebugManager.h"
#include "algo.h"
#include "Hmi.h"
#include "ParticlesManager.h"
#include "EspBridge.h"
#include "ParamFile.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MAJOR_VER 1
#define MINOR_VER 0
#define REVISION_VER 8

#define ESP32_ISACTIVE (1)
#define MEASURE_PARTICLES_ISACTIVE (1)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

RTC_HandleTypeDef hrtc;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_usart2_rx;

osThreadId defaultTaskHandle;
osTimerId TimerHandle;
/* USER CODE BEGIN PV */
static osThreadId TemperatureMeasHandle;
static osThreadId StepperManagerTHandle;
static osThreadId DebugManagerTHandle;
static osThreadId HmiManagerTHandle;

#if MEASURE_PARTICLES_ISACTIVE
static osThreadId ParticlesManagerTHandle;
#endif

#if ESP32_ISACTIVE
osThreadId EspManagerTHandle;
#endif

RTC_TimeTypeDef sTime;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_RTC_Init(void);
static void MX_USART1_UART_Init(void);
void StartDefaultTask(void const * argument);
void TimerCallback(void const * argument);

/* USER CODE BEGIN PFP */
//void TemperatureManager(void const * argument);
//void Steppermanager(void const * argument);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN 	Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_RTC_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  ESPMANAGER_Init();

  PARAMFILE_Init(); // Initialize param file reader before main process
  // Print all parameters into the debug file
  for(uint32_t ix = 0; ix < PARAMFILE_GetParamEntryCount(); ix++)
  {
	  const PFL_SParameterItem* pParamItem = PARAMFILE_GetParamEntryByIndex(ix);
	  if (pParamItem == NULL)
		  continue;

	  char tmp[128+1];
	  int32_t s32Value;
	  PFL_GetValueInt32(&PARAMFILE_g_sHandle, pParamItem->szKey, &s32Value);
	  snprintf(tmp, sizeof(tmp), "%s | %d (default: %d, min: %d, max: %d)", pParamItem->szKey, (int)s32Value, (int)pParamItem->uType.sInt32.s32Default, (int)pParamItem->uType.sInt32.s32Min, (int)pParamItem->uType.sInt32.s32Max);
	  printf(tmp);
  }

  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* definition and creation of Timer */
  osTimerDef(Timer, TimerCallback);
  TimerHandle = osTimerCreate(osTimer(Timer), osTimerPeriodic, NULL);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */

  osThreadDef(TemperatureMeas, TemperatureManager, osPriorityNormal, 0, 512); //TODO: Validate maximum stack needed adding printf end in Hard Fault handler
  TemperatureMeasHandle = osThreadCreate(osThread(TemperatureMeas), NULL);

  osThreadDef(StepperManagerT, Steppermanager, osPriorityNormal, 0, 128);
  StepperManagerTHandle = osThreadCreate(osThread(StepperManagerT), NULL);

  osThreadDef(DebugManagerT, DebugManager, osPriorityNormal, 0, 512);
  DebugManagerTHandle = osThreadCreate(osThread(DebugManagerT), NULL);

  osThreadDef(HmiManagerT, HmiManager, osPriorityNormal, 0, 128);
  HmiManagerTHandle = osThreadCreate(osThread(HmiManagerT), NULL);

#if MEASURE_PARTICLES_ISACTIVE
  osThreadDef(ParticlesManagerT, ParticlesManager, osPriorityNormal, 0, 128);
  ParticlesManagerTHandle = osThreadCreate(osThread(ParticlesManagerT), NULL);
#endif

#if ESP32_ISACTIVE
  osThreadDef(EspManagerT, ESPMANAGER_Task, osPriorityNormal, 0, 128);
  EspManagerTHandle = osThreadCreate(osThread(EspManagerT), NULL);
#endif

  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.Prediv1Source = RCC_PREDIV1_SOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL2.PLL2State = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure the Systick interrupt time
  */
  __HAL_RCC_PLLI2S_ENABLE();
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef DateToUpdate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */
  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
  hrtc.Init.OutPut = RTC_OUTPUTSOURCE_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;

  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  DateToUpdate.WeekDay = RTC_WEEKDAY_MONDAY;
  DateToUpdate.Month = RTC_MONTH_JANUARY;
  DateToUpdate.Date = 0x1;
  DateToUpdate.Year = 0x0;

  if (HAL_RTC_SetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */
#if ESP32_ISACTIVE
  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */
#endif
  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */
#if MEASURE_PARTICLES_ISACTIVE
  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 19200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */
#endif
  /* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, Step2_LowCurrent_Pin|uc_Stepper_Sleep_Pin|SPEED2_COIL_Pin|SPEED3_COIL_Pin
                          |Step2_DIR_Pin|Step1_LowCurrent_Pin|Step2_STEP_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, Step3_RESET_Pin|STATUS_LED1_Pin|Step3_ENABLE_Pin|Step2_RESET_Pin
                          |Step2_ENABLE_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, Buzzer_ON_Pin|AFK_Var_Pin|USB_ENABLE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, STATUS_LED2_Pin|Step3_DIR_Pin|Button_LED_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, Step3_STEP_Pin|Step3_LowCurrent_Pin|Stepper_HalfStep_Pin|Step1_STEP_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Step1_DIR_GPIO_Port, Step1_DIR_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, Step1_RESET_Pin|Step1_ENABLE_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : Step2_LowCurrent_Pin Step3_RESET_Pin STATUS_LED1_Pin uc_Stepper_Sleep_Pin
                           Step3_ENABLE_Pin SPEED2_COIL_Pin SPEED3_COIL_Pin Step2_DIR_Pin
                           Step1_LowCurrent_Pin Step2_STEP_Pin Step2_RESET_Pin Step2_ENABLE_Pin */
  GPIO_InitStruct.Pin = Step2_LowCurrent_Pin|Step3_RESET_Pin|STATUS_LED1_Pin|uc_Stepper_Sleep_Pin
                          |Step3_ENABLE_Pin|SPEED2_COIL_Pin|SPEED3_COIL_Pin|Step2_DIR_Pin
                          |Step1_LowCurrent_Pin|Step2_STEP_Pin|Step2_RESET_Pin|Step2_ENABLE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : Limit_switch1_Pin Limit_switch2_Pin */
  GPIO_InitStruct.Pin = Limit_switch1_Pin|Limit_switch2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : Buzzer_ON_Pin STATUS_LED2_Pin Step3_DIR_Pin AFK_Var_Pin
                           Button_LED_Pin USB_ENABLE_Pin */
  GPIO_InitStruct.Pin = Buzzer_ON_Pin|STATUS_LED2_Pin|Step3_DIR_Pin|AFK_Var_Pin
                          |Button_LED_Pin|USB_ENABLE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : Limit_switch3_Pin */
  GPIO_InitStruct.Pin = Limit_switch3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Limit_switch3_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : Limit_switch_Door_Pin Thermostat_Input_Pin Safety_ON_Pin Interlock_Input_Pin
                           Button_Input_Pin USB_Fault_Pin */
  GPIO_InitStruct.Pin = Limit_switch_Door_Pin|Thermostat_Input_Pin|Safety_ON_Pin|Interlock_Input_Pin
                          |Button_Input_Pin|USB_Fault_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : Step3_STEP_Pin Step3_LowCurrent_Pin Stepper_HalfStep_Pin Step1_STEP_Pin
                           Step1_RESET_Pin Step1_ENABLE_Pin */
  GPIO_InitStruct.Pin = Step3_STEP_Pin|Step3_LowCurrent_Pin|Stepper_HalfStep_Pin|Step1_STEP_Pin
                          |Step1_RESET_Pin|Step1_ENABLE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : Step1_DIR_Pin */
  GPIO_InitStruct.Pin = Step1_DIR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Step1_DIR_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN 5 */
	printf("-------------------------------\n\r"); //TODO: if we remove this call, go to hardfault handler  or fail to execute

	    switch (readModel())
	    {
	    	  case HEATMAX:
	  	   	  printf("HeatCom CaddyAdv");
	  	  break;
	    	  case CADDY_ADVANCED:
	    		  printf("Caddy Advanced");
	  	  break;
	    	  case HEATPACK:
	    		  printf("Heatpack");
	  	  break;
	    	  case MINI_CADDY:
	    		  printf("Mini Caddy");
	    	  break;
	    	  case HEATPRO:
	    		  printf("HeatPro");
	  	  break;
	    	  case MAX_CADDY:
	    		  printf("Max Caddy");
	  	  break;
	    	  default:
	    		  printf("Invalid Model");
	    		break;
	    }
	    uint32_t j=0; //for a dumbass delay
	    //HAL_GPIO_WritePin(STATUS_LED0_GPIO_Port,STATUS_LED0_Pin,SET);
	    HAL_GPIO_WritePin(STATUS_LED1_GPIO_Port,STATUS_LED1_Pin,SET);
	    HAL_GPIO_WritePin(STATUS_LED2_GPIO_Port,STATUS_LED2_Pin,SET);
	    for(j=0;j<10000000;j++){asm("NOP");}
	    printf(" Version %i.%i.%i\n\r",MAJOR_VER,MINOR_VER,REVISION_VER);


	    int i=0;

	    for(i=0;i<MAJOR_VER;i++)
	    {
	  	  //HAL_GPIO_WritePin(STATUS_LED0_GPIO_Port,STATUS_LED0_Pin,RESET);
	  	  //for(j=0;j<5000000;j++){asm("NOP");}
	  	  //HAL_GPIO_WritePin(STATUS_LED0_GPIO_Port,STATUS_LED0_Pin,SET);
	  	  //for(j=0;j<5000000;j++){asm("NOP");}
	    }
	    for(i=0;i<MINOR_VER;i++)
	    {
	  	  HAL_GPIO_WritePin(STATUS_LED1_GPIO_Port,STATUS_LED1_Pin,RESET);
	  	  for(j=0;j<5000000;j++){asm("NOP");}
	  	  HAL_GPIO_WritePin(STATUS_LED1_GPIO_Port,STATUS_LED1_Pin,SET);
	  	  for(j=0;j<5000000;j++){asm("NOP");}
	    }
	    for(i=0;i<REVISION_VER;i++)
	    {
	  	  HAL_GPIO_WritePin(STATUS_LED2_GPIO_Port,STATUS_LED2_Pin,RESET);
	  	  for(j=0;j<5000000;j++){asm("NOP");}
	  	  HAL_GPIO_WritePin(STATUS_LED2_GPIO_Port,STATUS_LED2_Pin,SET);
	  	  for(j=0;j<5000000;j++){asm("NOP");}
	    }

  /* Infinite loop */
  for(;;)
  {

	  osDelay(1);
  }
  /* USER CODE END 5 */
}

/* TimerCallback function */
void TimerCallback(void const * argument)
{
  /* USER CODE BEGIN TimerCallback */
	osTimerStop(TimerHandle);
  /* USER CODE END TimerCallback */
}

 /**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	printf("Error Handler called");
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
