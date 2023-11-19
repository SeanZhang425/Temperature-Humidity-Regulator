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

/* USER CODE BEGIN PV */
uint8_t bits[5];

double humidity;
double temperature;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);

/* USER CODE BEGIN PFP */
void PinMode_Out();
void PinMode_In();

int Read_Sensor();
int ReadTempHum();

void Over_Temp_Alarm(void);
void Under_Temp_Alarm(void);
void Over_Hum_Alarm(void);
void Under_Hum_Alarm(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void PinMode_Out() {
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	
	GPIO_InitStruct.Pin = Sensor_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(Sensor_GPIO_Port, &GPIO_InitStruct);
}

void PinMode_In() {
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	
	GPIO_InitStruct.Pin = Sensor_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(Sensor_GPIO_Port, &GPIO_InitStruct);
}

int ReadTempHum() {
	// READ VALUES
	int result = Read_Sensor();

	bits[0] &= 0x3F;
	bits[2] &= 0x3F;

	// CONVERT AND STORE
	humidity = bits[0];
	temperature = bits[2];

	// TEST CHECKSUM
	uint8_t sum = bits[0] + bits[2];

	if (bits[4] != sum)
		return -1;

	return result;
}

int Read_Sensor() {
	// INIT BUFFERVAR TO RECEIVE DATA
	uint8_t mask = 128;
	uint8_t idx = 0;

	uint8_t data = 0;
	uint8_t state = 0;
	uint8_t pstate = 0;
	uint16_t zeroLoop = 400;
	uint16_t delta = 0;

	//leadingZeroBits = 40 - leadingZeroBits; // reverse counting...

	// REQUEST SAMPLE
	PinMode_Out();
	HAL_GPIO_WritePin(Sensor_GPIO_Port, Sensor_Pin, 0); // T-be
	HAL_Delay(18);
	HAL_GPIO_WritePin(Sensor_GPIO_Port, Sensor_Pin, 1); // T-go
	PinMode_In();

	uint16_t loopCount = 400 * 2;  // 200uSec max

	while (HAL_GPIO_ReadPin(Sensor_GPIO_Port, Sensor_Pin) != 0)
		if (--loopCount == 0) return -3;

	// GET ACKNOWLEDGE or TIMEOUT
	loopCount = 400;

	while (HAL_GPIO_ReadPin(Sensor_GPIO_Port, Sensor_Pin) == 0)  // T-rel
		if (--loopCount == 0) return -4;

	loopCount = 400;

	while (HAL_GPIO_ReadPin(Sensor_GPIO_Port, Sensor_Pin) == 1)  // T-reh
		if (--loopCount == 0) return -5;

	loopCount = 400;

	// READ THE OUTPUT - 40 BITS => 5 BYTES
	for (uint8_t i = 40; i != 0;)
	{
		// WAIT FOR FALLING EDGE
		state = HAL_GPIO_ReadPin(Sensor_GPIO_Port, Sensor_Pin);

		if (state == 0 && pstate != 0)
		{
			if (i > 1)
			{
				uint16_t min;

				if (zeroLoop < loopCount)
					min = zeroLoop;
				else
					min = loopCount;

				zeroLoop = min;
				delta = (400 - zeroLoop) / 4;
			}
			else if (loopCount <= (zeroLoop - delta))
				data |= mask;

			mask >>= 1;

			if (mask == 0) // next byte
			{
				mask = 128;
				bits[idx++] = data;
				data = 0;
			}
			// next bit
			--i;

			// reset timeout flag
			loopCount = 400;
		}

		pstate = state;

		// Check timeout
		if (--loopCount == 0)
			return -2;

	}

	PinMode_Out();
	HAL_GPIO_WritePin(Sensor_GPIO_Port, Sensor_Pin, 1);

	return 0;
}

void Over_Temp_Alarm(void) {
	HAL_GPIO_WritePin(GreenTempLED_GPIO_Port, GreenTempLED_Pin, 1);

	for (int i = 0; i < 50; i++) {
		HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, 1);
		HAL_Delay(1);
		HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, 0);
		HAL_Delay(1);
	}

	HAL_Delay(100);
	HAL_GPIO_WritePin(GreenTempLED_GPIO_Port, GreenTempLED_Pin, 0);
	HAL_Delay(100);
}

void Under_Temp_Alarm(void) {
	HAL_GPIO_WritePin(RedTempLED_GPIO_Port, RedTempLED_Pin, 1);

	for (int i = 0; i < 50; i++) {
		HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, 1);
		HAL_Delay(2);
		HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, 0);
		HAL_Delay(2);
	}

	HAL_Delay(100);
	HAL_GPIO_WritePin(RedTempLED_GPIO_Port, RedTempLED_Pin, 0);
	HAL_Delay(100);
}

void Over_Hum_Alarm(void) {
	HAL_GPIO_WritePin(GreenHumLED_GPIO_Port, GreenHumLED_Pin, 1);

	for (int i = 0; i < 25; i++) {
		HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, 1);
		HAL_Delay(1);
		HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, 0);
		HAL_Delay(1);
	}

	HAL_Delay(10);

	for (int i = 0; i < 25; i++) {
		HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, 1);
		HAL_Delay(1);
		HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, 0);
		HAL_Delay(1);
	}

	HAL_Delay(100);
	HAL_GPIO_WritePin(GreenHumLED_GPIO_Port, GreenHumLED_Pin, 0);
	HAL_Delay(100);
}

void Under_Hum_Alarm(void) {
	HAL_GPIO_WritePin(RedHumLED_GPIO_Port, RedHumLED_Pin, 1);

	for (int i = 0; i < 25; i++) {
		HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, 1);
		HAL_Delay(2);
		HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, 0);
		HAL_Delay(2);
	}

	HAL_Delay(10);

	for (int i = 0; i < 25; i++) {
		HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, 1);
		HAL_Delay(2);
		HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, 0);
		HAL_Delay(2);
	}

	HAL_Delay(100);
	HAL_GPIO_WritePin(RedHumLED_GPIO_Port, RedHumLED_Pin, 0);
	HAL_Delay(100);
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
 
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      double tempSum = 0;
	  double humSum = 0;

	  for (int i = 0; i < 10; i++) {
		  ReadTempHum();

		  HAL_GPIO_WritePin(RedTempLED_GPIO_Port, RedTempLED_Pin, 1);
		  HAL_Delay(500);
		  HAL_GPIO_WritePin(RedTempLED_GPIO_Port, RedTempLED_Pin, 0);
		  HAL_Delay(500);

		  tempSum += temperature;
		  humSum += humidity;
		  HAL_Delay(30000);
	  }

	  double tempAvg = tempSum / 10.0;
	  double humAvg = humSum / 10.0;

	  GPIO_PinState Button_State = 0;
	  GPIO_PinState Prev_Button_State = 0;

	  while (1) {
		  if (tempAvg > 32)
			  Over_Temp_Alarm();
		  else if (tempAvg < 1)
			  Under_Temp_Alarm();

		  if (humAvg > 50)
			  Over_Hum_Alarm();
		  else if (humAvg < 30)
			  Under_Hum_Alarm();
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(RedTempLED_GPIO_Port, RedTempLED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GreenTempLED_Pin|GreenHumLED_Pin|RedHumLED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : Buzzer_Pin */
  GPIO_InitStruct.Pin = Buzzer_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Buzzer_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Sensor_Pin */
  GPIO_InitStruct.Pin = Sensor_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Sensor_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : RedTempLED_Pin */
  GPIO_InitStruct.Pin = RedTempLED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RedTempLED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : GreenTempLED_Pin GreenHumLED_Pin RedHumLED_Pin */
  GPIO_InitStruct.Pin = GreenTempLED_Pin|GreenHumLED_Pin|RedHumLED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : Button_Pin */
  GPIO_InitStruct.Pin = Button_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Button_GPIO_Port, &GPIO_InitStruct);

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
