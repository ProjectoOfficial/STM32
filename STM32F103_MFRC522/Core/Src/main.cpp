/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "MFRC522.hpp"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MAX_STRING_LENGTH 255
#define UID_LENGTH 16

#define HUMAN_DELAY 350
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
char rxbuffer[UID_LENGTH];
char new_uid[UID_LENGTH];
bool new_id_available = false;

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size){
	if(huart->Instance == USART1){
		char msg[] = "DMA has been called!\n";
		HAL_UART_Transmit(&huart1, (const uint8_t *) msg, strlen(msg), HAL_MAX_DELAY);

		memcpy(new_uid, rxbuffer, UID_LENGTH);
		new_id_available = true;
	    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, (uint8_t*) rxbuffer, MAX_STRING_LENGTH);
	    __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
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
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
//  HAL_UART_RegisterCallback(&huart1, HAL_UART_RX_CPLT_CB_ID, HAL_UART_RxCpltCallback);

  char message[MAX_STRING_LENGTH] = "STM32F103C8T6 Initialized!\n";

  HAL_UART_Transmit(&huart1, (const uint8_t *) message, strlen(message), HAL_MAX_DELAY);

  MFRC522 rfid = MFRC522(&hspi1, CS_GPIO_Port, CS_Pin, RESET_GPIO_Port, RESET_Pin);
  rfid.PCD_Init();

  bool result = rfid.PCD_PerformSelfTest();
  if (result){
	  sprintf(message, "MFRC522 is working properly! Now you can read your TAG.\n");
	  HAL_UART_Transmit(&huart1, (const uint8_t *) message, strlen(message), HAL_MAX_DELAY);
  }
  else{
	  sprintf(message, "MFRC522 encountered some errors during protocol initialization\n");
	  HAL_UART_Transmit(&huart1, (const uint8_t *) message, strlen(message), HAL_MAX_DELAY);
	  Error_Handler();
  }

  uint8_t version = rfid.PCD_DumpVersion();
  sprintf(message, "MFRC522 Version: %u\n", version);
  HAL_UART_Transmit(&huart1, (const uint8_t *) message, strlen(message), HAL_MAX_DELAY);

  /* USER CODE BEGIN 2 */
  char uid[UID_LENGTH];
  bool write_mode = false;

  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, (uint8_t*) rxbuffer, MAX_STRING_LENGTH);
  __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT); // half data transfer interrupt disable

  MFRC522::MIFARE_Key key;
  for (uint8_t i = 0; i < 6; i++) {
	  key.keyByte[i] = 0xFF;
  }


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	if (HAL_GPIO_ReadPin(WRITE_BTN_GPIO_Port, WRITE_BTN_Pin) == GPIO_PIN_SET){
		write_mode = !write_mode;
		if (write_mode){
			sprintf(message, "WRITE MODE: ON\n");
		}
		else{
			sprintf(message, "WRITE MODE: OFF\n");
		}
		HAL_UART_Transmit(&huart1, (const uint8_t *) message, strlen(message), HAL_MAX_DELAY);
		HAL_Delay(HUMAN_DELAY);
	}

	if (new_id_available){
		sprintf(message, "RX data received: %s\n", new_uid);
		HAL_UART_Transmit(&huart1, (const uint8_t *) message, strlen(message), HAL_MAX_DELAY);
		new_id_available = false;
	}

	if(rfid.PICC_IsNewCardPresent()){
		if(rfid.PICC_ReadCardSerial()){
			HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
			char rfid_uid[16];
			int uid_length = 0;

			for(int i = 0; i < rfid.uid.size; i++){
	//			uid += rfid.uid.uidByte[i] < 0x10 ? "0" : "";
	//			uid += String(rfid.uid.uidByte[i], HEX);
				uid_length += snprintf(uid + uid_length, sizeof(uid) - uid_length, "%02X", rfid_uid[i]);
			}
		    sprintf(message, "New TAG detected: %s\n", uid);
			HAL_UART_Transmit(&huart1, (const uint8_t *) message, strlen(message), HAL_MAX_DELAY);
			HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

			if (write_mode){
				sprintf(message, "Setting new TAG ID as: %s...\n", new_uid);
				HAL_UART_Transmit(&huart1, (const uint8_t *) message, strlen(message), HAL_MAX_DELAY);
				if (rfid.MIFARE_SetUid((uint8_t *)new_uid, (uint8_t) strlen(new_uid), true)){
					sprintf(message, "TAG ID %s written successfully!\n", new_uid);
					HAL_UART_Transmit(&huart1, (const uint8_t *) message, strlen(message), HAL_MAX_DELAY);
				}
				else{
					sprintf(message, "could not write ID %s on the TAG\n", new_uid);
						HAL_UART_Transmit(&huart1, (const uint8_t *) message, strlen(message), HAL_MAX_DELAY);
				}
			}
			rfid.PICC_HaltA();
			HAL_Delay(HUMAN_DELAY);
		}
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

	/** Initializes the RCC Oscillators according to the specified parameters
	* in the RCC_OscInitTypeDef structure.
	*/
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
	Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	*/
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
							  |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
	{
	Error_Handler();
	}
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
	hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
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
  huart1.Init.BaudRate = 9600;
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
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

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
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : WRITE_BTN_Pin */
  GPIO_InitStruct.Pin = WRITE_BTN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(WRITE_BTN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : CS_Pin */
  GPIO_InitStruct.Pin = CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CS_GPIO_Port, &GPIO_InitStruct);

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

  char msg[] = "Ooops, we encountered some errors! I crashed.";
  HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);

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
