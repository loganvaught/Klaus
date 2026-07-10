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
#include "drv2605l.h"
#include "nrf24l01.h"
#include "metronome.h"
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define RF_CHANNEL 92
#define DRV2605L_WAVEFORM_USE 4

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim5;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM5_Init(void);
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

uint8_t nrf_irq_pending = 0;
uint8_t btn_pressed = 0;
// Setup callbacks for buttons, tempo knob, RF interrupt pin
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == ENCODER_A_Pin || GPIO_Pin == ENCODER_B_Pin) {
		// Encoder A pin
		encoder_change = 1;
	}
	else if (GPIO_Pin == NRF_IRQ_Pin) {
		// NRF Interrupt pin
		// Active low.
		// Set flag to handle the IRQ
		nrf_irq_pending = 1;
		// Automatically set back to high when status register IRQs are cleared
	}
	else if (GPIO_Pin == BTN_Pin) {
		// User button pressed, indicating NRF mode change.
		btn_pressed = 1;
	}
}

// NRF24L01 code
// Array used for transmitting master timestamp and BPM in TX, or receiving in RX
uint8_t rf_buff[6] = {
		// 1 = BPM, MSB
		// 2 = BPM, LSB
		// 3 = Master timestamp, MSB
		// 6 = Master timestamp, LSB
};

// Callback to process received information
uint8_t packet_received = 0;
void rx_callback(uint8_t *data, uint16_t length) {
	if (length == (sizeof(rf_buff) / sizeof(rf_buff[0]))) {
		for (int i = 0; i < length; i ++) {
			rf_buff[i] = data[i];
		}
		packet_received = 1;
	}
	else {
		printf("Received data of unexpected length.\r\n");
		Error_Handler();
	}
}

// Callback for a successful transmit
uint8_t transmit_flag = 0;
void tx_callback() {
  transmit_flag = 1;
}

drv2605l_handle_t haptic_driver;
nrf24l01_handle_t rf_handle;
metronome_t metronome;

// Callback for metronome pulse
// Blink on-board LED for visual debugging
void pulse_callback(uint16_t tempo, uint32_t timestamp) {
	if (rf_handle.mode != NRF24L01_RX) {
		HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
	}

	// Play the vibration waveform (4 is for sharp short click)
	drv2605l_play(&haptic_driver, DRV2605L_WAVEFORM_USE);

	// If in TX mode, transmit timestamp and tempo
	if (rf_handle.mode == NRF24L01_TX) {
		// Fill rf buffer
		rf_buff[0] = tempo >> 8;
		rf_buff[1] = tempo & 0xFF;
		rf_buff[2] = timestamp >> 24;
		rf_buff[3]= (timestamp >> 16) & 0xFF;
		rf_buff[4]= (timestamp >> 8) & 0xFF;
		rf_buff[5]= timestamp & 0xFF;

		nrf24l01_result_t nrf24l01_result = nrf24l01_transmit(&rf_handle, rf_buff, 6);
		if (nrf24l01_result != NRF24L01_OK) {
			printf("Error when transmitting\r\n");
		}
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
  MX_SPI1_Init();
  MX_TIM3_Init();
  MX_TIM5_Init();

  /* USER CODE BEGIN 2 */

  printf("\r\nWorking!\r\n");

  // -------------- DRV2605L setup -------------- //

  drv2605l_init(&haptic_driver, &hi2c1);

  /* CODE TO CYCLE THROUGH ALL THE VIBRATION WAVEFORMS
  for (uint8_t i = 1; i < 124; i ++) {
	  HAL_Delay(250);
	  printf("Playing: %u\r\n",i);
	  drv2605l_play(i);
  }*/

  // -------------- NRF24L01+ setup -------------- //

  micro_delay_handle_t micro_timer;
  HAL_TIM_Base_Start(&htim3); // The timer to be used with the microsecond wait
  micro_delay_init(&micro_timer, &htim3);
  uint8_t channel = RF_CHANNEL;
  nrf24l01_datarate_t data_rate = NRF24L01_DR_250KBPS;
  nrf24l01_power_t power = NRF24L01_PWR_0DBM;
  uint8_t pipe_address[5] = {0x55,0x4E,0x55,0x4E,0x55,};

  // Initialize nrf
  nrf24l01_result_t nrf24l01_result;
  nrf24l01_result = nrf24l01_init(&rf_handle, &hspi1, &micro_timer, NRF_CS_GPIO_Port, NRF_CS_Pin, NRF_CE_GPIO_Port, NRF_CE_Pin, channel, data_rate, power, pipe_address);
  if (nrf24l01_result != NRF24L01_OK) {
	  printf("Error when initializing nrf24l01\r\n");
	  Error_Handler();
  }
  else {
	  printf("Instantiated right!\r\n");
  }

  // Init success. Set callbacks
  rf_handle.rx_callback = rx_callback;
  rf_handle.tx_callback = tx_callback;

  // -------------- Metronome setup -------------- //

  // Start timer for metronome beats. 32 bit
  HAL_TIM_Base_Start_IT(&htim5);
  metronome_init(&metronome, &htim5);
  metronome.pulse_callback = pulse_callback;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
	  //---------------------------- ENCODER CODE ------------------------------//
	  if (encoder_change == 2) { // First time initializing encoder variables
		  encoder_change = 0;
		  encoder_state = (HAL_GPIO_ReadPin(ENCODER_A_GPIO_Port, ENCODER_A_Pin) << 1) | HAL_GPIO_ReadPin(ENCODER_B_GPIO_Port, ENCODER_B_Pin);
		  encoder_last_state = encoder_state;
	  }

	  if (encoder_change) {
		  // Rotary encoder twisted
		  // Make sure not in RX mode.
		  if (rf_handle.mode != NRF24L01_RX) {
			  encoder_state = (HAL_GPIO_ReadPin(ENCODER_A_GPIO_Port, ENCODER_A_Pin) << 1) | HAL_GPIO_ReadPin(ENCODER_B_GPIO_Port, ENCODER_B_Pin);
			  metronome_direction_t direction;
			  if (encoder_clockwise_dict[encoder_last_state] == encoder_state) {
				  // Go clockwise
				  direction = METRONOME_FORWARD;
			  }
			  else if (encoder_counterclockwise_dict[encoder_last_state] == encoder_state) {
				  // Go counterclockwise
				  direction = METRONOME_BACKWARD;
			  }
			  else {
				  // Noise, ignore
				  direction = METRONOME_NONE;
			  }

			  if (encoder_state == 0) { // The encoder is detented, so this checks for a complete twist.
				  metronome_change_tempo(&metronome, direction);
			  }

			  encoder_last_state = encoder_state;
		  }
		  encoder_change = 0;
	  }

	  //---------------------------- nRF24L01+ CODE ------------------------------//

	  // Handle interrupts
	  if (nrf_irq_pending) {
		  // An IRQ was set high in the status register. Handle it
		  nrf24l01_result = nrf24l01_handle_irqs(&rf_handle);
		  if (nrf24l01_result != NRF24L01_OK) {
			  Error_Handler();
		  }
		  nrf_irq_pending = 0;
	  }

	  // Handle mode changes
	  if (btn_pressed) {
		  btn_pressed = 0;

		  // Reset the offset, and re-initialize pulse timestamps
		  metronome_reset_rf(&metronome);
		  switch (rf_handle.mode) {
		  case NRF24L01_RX: // Entering Off mode
			  nrf24l01_enter_off(&rf_handle);
			  HAL_GPIO_WritePin(RF_LED_TX_GPIO_Port, RF_LED_TX_Pin, GPIO_PIN_RESET);
			  HAL_GPIO_WritePin(RF_LED_RX_GPIO_Port, RF_LED_RX_Pin, GPIO_PIN_RESET);
			  break;
		  case NRF24L01_Off: // Entering TX mode
			  nrf24l01_enter_tx(&rf_handle);
			  HAL_GPIO_WritePin(RF_LED_TX_GPIO_Port, RF_LED_TX_Pin, GPIO_PIN_SET);
			  HAL_GPIO_WritePin(RF_LED_RX_GPIO_Port, RF_LED_RX_Pin, GPIO_PIN_RESET);
			  break;
		  case NRF24L01_TX: // Entering RX
			  nrf24l01_enter_rx(&rf_handle);
			  HAL_GPIO_WritePin(RF_LED_RX_GPIO_Port, RF_LED_RX_Pin, GPIO_PIN_SET);
			  HAL_GPIO_WritePin(RF_LED_TX_GPIO_Port, RF_LED_TX_Pin, GPIO_PIN_RESET);
			  break;
		  default:
			  printf("Invalid button mode\r\n");
			  Error_Handler();
		  }
	  }

	  //---------------------------- METRONOME CODE ------------------------------//

	  // Check for initialization of timestasmps
	  // Check if time to pulse
	  metronome_update(&metronome);

	  // Handle RX
	  if (packet_received) {
		  packet_received = 0;
		  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

		  // Decode the packet
		  uint16_t tempo = rf_buff[0] << 8 | rf_buff[1];
		  uint32_t master_timestamp = rf_buff[2] << 24 | rf_buff[3] << 16 | rf_buff[4] << 8 | rf_buff[5];

		  metronome_process_rf(&metronome, tempo, master_timestamp);
	  }

	  // Handle TX
	  if (transmit_flag) {
		  transmit_flag = 0;
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
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 100 - 1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief TIM5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 10000 - 1;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 4294967295;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, NRF_CE_Pin|NRF_CS_Pin|RF_LED_RX_Pin|RF_LED_TX_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : BTN_Pin ENCODER_A_Pin ENCODER_B_Pin */
  GPIO_InitStruct.Pin = BTN_Pin|ENCODER_A_Pin|ENCODER_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : NRF_CE_Pin NRF_CS_Pin RF_LED_RX_Pin RF_LED_TX_Pin */
  GPIO_InitStruct.Pin = NRF_CE_Pin|NRF_CS_Pin|RF_LED_RX_Pin|RF_LED_TX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : NRF_IRQ_Pin */
  GPIO_InitStruct.Pin = NRF_IRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(NRF_IRQ_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

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
