/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define HAPTIC_DEV_ADDR 0x5A
#define HAPTIC_MODE_REG 0x01
#define TEMPO_MAX 208
#define TEMPO_MIN 30

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Code to redirect printf through UART.
int _write(int fd, char* ptr, int len) {
	HAL_StatusTypeDef hstatus;

	if (fd == 1 || fd == 2) {
	  hstatus = HAL_UART_Transmit(&huart1, (uint8_t *) ptr, len, HAL_MAX_DELAY);
	  if (hstatus == HAL_OK)
		return len;
	  else
		return -1;
	}
	return -1;
}

void haptic_write_byte(uint8_t mAddr, uint8_t data) {
	if (HAL_I2C_Mem_Write(&hi2c1, HAPTIC_DEV_ADDR << 1, mAddr, 1, &data, 1, HAL_MAX_DELAY) != HAL_OK) {
		printf("Haptic write failed.\r\n");
		Error_Handler();
	}
}

uint8_t haptic_read_byte(uint8_t mAddr) {
	uint8_t data = 0;
	if (HAL_I2C_Mem_Read(&hi2c1, HAPTIC_DEV_ADDR << 1, mAddr, 1, &data, 1, HAL_MAX_DELAY) != HAL_OK) {
		printf("Haptic read failed.\r\n");
		Error_Handler();
	}
	return data;
}

// Helper function to set up the bits required for the auto-callibration process. Addresses and values from datasheet and formulas in datasheet
void haptic_autoconfig() {
	// ERM_LRA[0] - 0x1A[7]. SET TO 1 FOR LRA
	uint8_t data = haptic_read_byte(0x1A);
	data = data | 0x80;
	haptic_write_byte(0x1A, data);

	// FB_BRAKE_FACTOR[2:0] - 0x1A[6:4].
	// LOOP_GAIN[1:0] - 0x1A[3:2]
	// RATED_VOLTAGE[7:0] - 0x16[7:0]. SET TO 0x56 FOR 2.0VRms (See formula on DRV datasheet)
	haptic_write_byte(0x16, 0x56);

	// OD_CLAMP[7:0] - 0x17[7:0]. SET TO 0x84 FOR 2.8V clamp. (See formula on DRV datasheet)
	haptic_write_byte(0x17, 0x84);

	// AUTO_CAL_TIME[1:0] - 0x1E[5:4]
	// DRIVE_TIME[4:0] - 0x1B[4:0]. SET TO 0x1C FOR 150Hz
	data = haptic_read_byte(0x1B);
	data = data & 0xE0; // Remove 5 least significant bits
	data = data | 0x1C;
	haptic_write_byte(0x1B, data);

	// SAMPLE_TIME[1:0] - 0x1C[5:4]
	// BLANKING_TIME[3:2] - 0x1F[3:2]
	// BLANKING_TIME[1:0] - 0x1C[3:2]
	// IDISS_TIME[3:2] - 0x1F[1:0]
	// IDISS_TIME[1:0] - 0x1C[1:0]
	// ZC_DET_TIME[1:0] - 0x1E[7:6]
}

void haptic_callibrate() {
	// Set configuration mode, remove from standby
	haptic_write_byte(0x01, 0x07);

	// Set values for my specific LRA: ELV1411A
	haptic_autoconfig();

	// Begin Auto-Callibration. Set GO bit
	haptic_write_byte(0x0C, 0x01);

	// Poll the go-bit to detect when callibration is completed
	uint8_t finished = 0;
	uint8_t checks = 0;
	uint8_t max_checks = 10;
	while (finished == 0 && checks < max_checks){
		HAL_Delay(100);
		if (haptic_read_byte(0x0C) == 0x00) finished = 1;
		checks ++;
	}

	printf("Checks completed\r\n");

	if (checks >= max_checks) printf("Auto-callibration time exhausted.\r\n");
	uint8_t result = (haptic_read_byte(0x00) & 0x08 );
	// Check whether or not the DIAG_RESULT bit shows successful completion
	if (result == 1) printf("Auto-callibration failed.\r\n");
}

// Initialize the DRV2065L as specified in the datasheet
void haptic_init() {
	// Wait for DRV2065L to be ready
	HAL_Delay(500);

	// Calibrate
	haptic_callibrate();

	// Remove calibration mode.
	haptic_write_byte(HAPTIC_MODE_REG, 0x00);

	// Library selection. LRA = Library No. 6
	uint8_t data = haptic_read_byte(0x03);
	uint8_t mask = 0xF8; // Mask to remove the 3 least significant bits
	data = data & mask;
	data = data | 0x06;
	haptic_write_byte(0x03, data);

	// Set up for vibration
	data = haptic_read_byte(0x01);
	mask = 0xF8; // Mask to remove the 3 least significant bits
	data = data & mask;
	data = data | 0x00; // Select "Internal Trigger"
	haptic_write_byte(0x01, data);
}

// Plays the numth waveform in the LRA library. Accepts a number from 1 to 123
void haptic_play(uint8_t num) {
	if (num < 1 || num > 123) {
		printf("Invalid waveform identifier. Skipping: %u\r\n", num);
		return;
	}

	// Check if go bit is 0
	if (haptic_read_byte(0x0C) != 0) return;

	haptic_write_byte(0x04, num);

	// Set GO bit
	haptic_write_byte(0x0C, 0x01);

	/* Poll the go-bit to detect when vibration is completed
	uint8_t finished = 0;
	uint8_t checks = 0;
	uint8_t max_checks = 100;
	while (finished == 0 && checks < max_checks){
		HAL_Delay(100);
		if (haptic_read_byte(0x0C) == 0x00) finished = 1;
		checks ++;
	}
	*/
}

uint8_t encoder_last_state = 0;
uint8_t encoder_state = 0;
uint8_t encoder_change = 2;

// Going Clockwise: 00, 10, 11, 01, 00 -- 0, 2, 3, 1, 0
// If encoder_clockwise_dict[last_state] == new state, you go clockwise.
uint8_t encoder_clockwise_dict[4] = {
		2, 0, 3, 1
};
// If encoder_counterclockwise_dict[last_state] == new state, you go counterclockwise.
uint8_t encoder_counterclockwise_dict[4] = {
		1, 3, 0, 2
};
// If the new value is in neither, it is just noise

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == ENCODER_A_Pin || GPIO_Pin == ENCODER_B_Pin) {
		// Encoder A pin
		encoder_change = 1;
	}
}

// Timer for generating metronome beats

uint8_t metronome_beat = 0;
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM2) {
		// Timer 2 reset
		metronome_beat = 1;
	}
}

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
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  printf("Working!\r\n");
  haptic_init();

  // Start timer for generating metronome beats
  HAL_TIM_Base_Start_IT(&htim2);

  /* CODE TO CYCLE THROUGH ALL THE VIBRATION WAVEFORMS
  for (uint8_t i = 1; i < 124; i ++) {
	  HAL_Delay(250);
	  printf("Playing: %u\r\n",i);
	  haptic_play(i);
  }*/

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  int16_t tempo = 60;

  while (1)
  {
	  // ENCODER CODE
	  if (encoder_change == 2) { // First time initializing encoder variables
		  encoder_change = 0;
		  encoder_state = (HAL_GPIO_ReadPin(ENCODER_A_GPIO_Port, ENCODER_A_Pin) << 1) | HAL_GPIO_ReadPin(ENCODER_B_GPIO_Port, ENCODER_B_Pin);
		  encoder_last_state = encoder_state;
	  }

	  if (encoder_change) {
		  // Rotary encoder twisted
		  encoder_state = (HAL_GPIO_ReadPin(ENCODER_A_GPIO_Port, ENCODER_A_Pin) << 1) | HAL_GPIO_ReadPin(ENCODER_B_GPIO_Port, ENCODER_B_Pin);
		  int8_t change = 0;
		  if (encoder_clockwise_dict[encoder_last_state] == encoder_state) {
			  // Go clockwise
			  change = 1;
		  }
		  else if (encoder_counterclockwise_dict[encoder_last_state] == encoder_state) {
			  // Go counterclockwise
			  change = -1;
		  }
		  else {
			  // Noise, ignore
		  }

		  if (encoder_state == 0) { // The encoder is detented, so this checks for a complete twist.
			  uint16_t new_tempo = tempo + change;

			  // Make sure tempo in range
			  // And, multiply change to make twisting knob faster for higher tempos

			  if (new_tempo >= 30 && new_tempo < 60) {
				  new_tempo += 1 * change;
				  tempo = new_tempo;
			  }
			  else if (new_tempo >= 60 && new_tempo < 72) {
				  new_tempo += 2 * change;
				  tempo = new_tempo;
			  }
			  else if (new_tempo >= 72 && new_tempo < 120) {
				  new_tempo += 3 * change;
				  tempo = new_tempo;
			  }
			  else if (new_tempo >= 120 && new_tempo < 144) {
				  new_tempo += 5 * change;
				  tempo = new_tempo;
			  }
			  else if (new_tempo >= 144 && new_tempo < 208) {
				  new_tempo += 7 * change;
				  tempo = new_tempo;
			  }

			  // Calculate new counter period for timer with new tempo
			  uint16_t new_period = 600000 / tempo;
			  printf("New period: %d. New tempo: %d\r\n", new_period, tempo);
			  __HAL_TIM_SET_COUNTER(&htim2, 0);
			  __HAL_TIM_SET_AUTORELOAD(&htim2, new_period - 1);
		  }

		  encoder_last_state = encoder_state;
		  encoder_change = 0;

	  }

	  // TIMER CODE

	  if (metronome_beat) {
		  // Play vibration waveform for beat
		  haptic_play(1);
		  metronome_beat = 0;
	  }

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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 12;
  RCC_OscInitStruct.PLL.PLLN = 96;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
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
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 10000 - 1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 10000 - 1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

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
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pins : ENCODER_B_Pin ENCODER_A_Pin */
  GPIO_InitStruct.Pin = ENCODER_B_Pin|ENCODER_A_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
