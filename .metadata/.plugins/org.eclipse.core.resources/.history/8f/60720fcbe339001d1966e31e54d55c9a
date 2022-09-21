/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "MotorManager.h"
#include "TemperatureManager.h"
#include "DebugPort.h"
#include "TestManager.h"
#include "DebugManager.h"
#include "algo.h"
#include "Hmi.h"
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
I2C_HandleTypeDef hi2c1;
RTC_HandleTypeDef hrtc;
UART_HandleTypeDef huart3;

osThreadId defaultTaskHandle;
osThreadId TemperatureMeasHandle;
osThreadId StepperManagerTHandle;
osThreadId DebugManagerTHandle;
osThreadId HmiManagerTHandle;

osTimerId TimerHandle;
osSemaphoreId I2CSemaphoreHandle;
/* USER CODE BEGIN PV */
uint32_t uCAdcData[2], buffer[2], temperature;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
void MX_I2C1_Init(void);
static void MX_RTC_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_ADC1_Init(void);
void StartDefaultTask(void const * argument);
void TemperatureManager(void const * argument);
void Steppermanager(void const * argument);

#define MAJOR_VER 1
#define MINOR_VER 0
#define REVISION_VER 8

RTC_TimeTypeDef sTime;
void TimerCallback(void const * argument);

/* USER CODE BEGIN PFP */

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

  /* USER CODE BEGIN Init */

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
  MX_USART3_UART_Init();
  MX_ADC1_Init();
  while(HAL_ADCEx_Calibration_Start(&hadc1)!=HAL_OK);
  //MX_USB_PCD_Init();
  /* USER CODE BEGIN 2 */

  HAL_ADC_Start_DMA(&hadc1, buffer, 2);



  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of I2CSemaphore */
  osSemaphoreDef(I2CSemaphore);
  I2CSemaphoreHandle = osSemaphoreCreate(osSemaphore(I2CSemaphore), 1);

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
  osThreadDef(TemperatureMeas, TemperatureManager, osPriorityNormal, 0, 512); //TODO: Validate maximum stack needed adding printf end in Hard Fault handler
  TemperatureMeasHandle = osThreadCreate(osThread(TemperatureMeas), NULL);

  osThreadDef(StepperManagerT, Steppermanager, osPriorityNormal, 0, 128);
  StepperManagerTHandle = osThreadCreate(osThread(StepperManagerT), NULL);

  osThreadDef(DebugManagerT, DebugManager, osPriorityNormal, 0, 512);
  DebugManagerTHandle = osThreadCreate(osThread(DebugManagerT), NULL);

  osThreadDef(HmiManagerT, HmiManager, osPriorityNormal, 0, 128);
  HmiManagerTHandle = osThreadCreate(osThread(HmiManagerT), NULL);

  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
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
  HAL_GPIO_WritePin(STATUS_LED0_GPIO_Port,STATUS_LED0_Pin,SET);
  HAL_GPIO_WritePin(STATUS_LED1_GPIO_Port,STATUS_LED1_Pin,SET);
  HAL_GPIO_WritePin(STATUS_LED2_GPIO_Port,STATUS_LED2_Pin,SET);
  for(j=0;j<10000000;j++){asm("NOP");}
  printf(" Version %i.%i.%i\n\r",MAJOR_VER,MINOR_VER,REVISION_VER);


  int i=0;

  for(i=0;i<MAJOR_VER;i++)
  {
	  HAL_GPIO_WritePin(STATUS_LED0_GPIO_Port,STATUS_LED0_Pin,RESET);
	  for(j=0;j<5000000;j++){asm("NOP");}
	  HAL_GPIO_WritePin(STATUS_LED0_GPIO_Port,STATUS_LED0_Pin,SET);
	  for(j=0;j<5000000;j++){asm("NOP");}
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

//#endif

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
#define NB_SAMPLES 30
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	static uint32_t filteredVoltage[NB_SAMPLES][2];
	static int j=0;
	for (int i =0; i<2; i++)
	{
		filteredVoltage[j][i] = buffer[i];
		//uCAdcData[i] = buffer[i];
	}
	j++;
	if(j>NB_SAMPLES)
	{
		j=0;
	}
	uint32_t sum = 0;
	for (int i =0; i<2; i++)
	{
		for (int k=0; k<NB_SAMPLES;k++)
		{
			sum += filteredVoltage[k][i];
		}
		uCAdcData[i] = sum/NB_SAMPLES;
		sum=0;
	}
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

  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_ADC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV4;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */
  /** Common config 
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 2;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Regular Channel 
  */
  sConfig.Channel = ADC_CHANNEL_6;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Regular Channel 
  */
  sConfig.Channel = ADC_CHANNEL_7;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
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
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

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
  HAL_GPIO_WritePin(GPIOC, step2_LowCurrent_Pin|Remote_led_Pin|uc_Stepper_Sleep_Pin|STATUS_LED0_Pin 
                          |STATUS_LED1_Pin|STATUS_LED2_Pin|Step2_DIR_Pin|Step1_LowCurrent_Pin 
                          |Step2_STEP_Pin|Step2_RESET_Pin|Step2_ENABLE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SPEED3_COIL_Pin|SPEED2_COIL_Pin|SPEED1_COIL_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, Button_LED_Pin|USB_LED_Pin|Step1_STEP_Pin|Step1_RESET_Pin 
                          |Step1_ENABLE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Step1_DIR_GPIO_Port, Step1_DIR_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : step2_LowCurrent_Pin Remote_led_Pin uc_Stepper_Sleep_Pin STATUS_LED0_Pin 
                           STATUS_LED1_Pin STATUS_LED2_Pin Step2_DIR_Pin Step1_LowCurrent_Pin 
                           Step2_STEP_Pin Step2_RESET_Pin Step2_ENABLE_Pin */
  GPIO_InitStruct.Pin = step2_LowCurrent_Pin|Remote_led_Pin|uc_Stepper_Sleep_Pin|STATUS_LED0_Pin 
                          |STATUS_LED1_Pin|STATUS_LED2_Pin|Step2_DIR_Pin|Step1_LowCurrent_Pin 
                          |Step2_STEP_Pin|Step2_RESET_Pin|Step2_ENABLE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LimitSwith1_Pin LimitSwitch2_Pin Remote_BTN_Pin */
  GPIO_InitStruct.Pin = LimitSwith1_Pin|LimitSwitch2_Pin|Remote_BTN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : SPEED3_COIL_Pin SPEED2_COIL_Pin SPEED1_COIL_Pin */
  GPIO_InitStruct.Pin = SPEED3_COIL_Pin|SPEED2_COIL_Pin|SPEED1_COIL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : Rev_bit1_Pin Rev_bit0_Pin Model_bit0_Pin Model_bit1_Pin 
                           Model_bit2_Pin */
  GPIO_InitStruct.Pin = Rev_bit1_Pin|Rev_bit0_Pin|Model_bit0_Pin|Model_bit1_Pin 
                          |Model_bit2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : Button_LED_Pin USB_LED_Pin Step1_STEP_Pin Step1_RESET_Pin 
                           Step1_ENABLE_Pin */
  GPIO_InitStruct.Pin = Button_LED_Pin|USB_LED_Pin|Step1_STEP_Pin|Step1_RESET_Pin 
                          |Step1_ENABLE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : Test_Pin_Pin Thermostat_input_Pin Safety_ON_Pin Interlock_input_Pin 
                           Button_input_Pin USB_Fault_Pin */
  GPIO_InitStruct.Pin = Test_Pin_Pin|Thermostat_input_Pin|Safety_ON_Pin|Interlock_input_Pin 
                          |Button_input_Pin|USB_Fault_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
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

  //if(GPIO_PIN_RESET==HAL_GPIO_ReadPin(Button_input_GPIO_Port,Button_input_Pin))
  //{
  //	  Algo_startChargement();
  // }
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
  /* User can add his own implementation to report the HAL error return state */

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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
