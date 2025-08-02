/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
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
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define UART_BUFFER_SIZE 16
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
volatile uint32_t last_rising = 0, rising = 0, falling = 0;
volatile uint8_t new_data_available = 0;

volatile char uart_buffer[UART_BUFFER_SIZE];
volatile uint8_t uart_index = 0;
volatile bool uart_number_ready = false;
uint8_t uart_rx_byte;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
static void RenderPwmUart(uint16_t duty, uint32_t freq);
static void UartSetCursorPosition(int row, int col);
static int ReadUARTNumber(void);
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
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  // Start Input Capture for CH1 (rising edge)
  HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
  // Start Input Capture for CH2 (falling edge)
  HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_2);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  setbuf(stdout, NULL); // so that we do not need to add \r\n at the end of printf statements
  int lastDuty = -10;
  bool signalAcquired = false;
  uint32_t lastPulse = 0;
  uart_rx_byte = 0;
  HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
  while (1)
  {
    uint32_t now = HAL_GetTick();

    int num = ReadUARTNumber();
    if (num >= 0)
    {
      char msg[32];
      sprintf(msg, "Got number: %d\r\n", num);
      printf(msg);
    }

    if (new_data_available)
    {
      new_data_available = 0;
      lastPulse = now;
      signalAcquired = true;
      uint32_t period = rising - last_rising;
      uint32_t high_time = falling - last_rising;
      uint32_t freq = 0;
      uint16_t duty = 0;

      if (period > 0)
      {
        freq = HAL_RCC_GetPCLK1Freq() * 2 / period;
        duty = (high_time * 100) / period;
        if (duty > 100)
          duty = duty - 100;
        if (duty > 100)
          continue;
      }
      if (freq < 1000)
        duty = 0;
      if (abs(lastDuty - duty) > 1)
      {
        RenderPwmUart(duty, freq);
        lastDuty = duty;
      }
    } else if (signalAcquired && now - lastPulse > 1000)
    {
      signalAcquired = false;
      RenderPwmUart(0, 0);
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
  RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
  RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

  /** Configure the main internal regulator output voltage
   */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

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
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
      | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
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
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
//static void MX_TIM2_Init(void)
//{
//
//  /* USER CODE BEGIN TIM2_Init 0 */
//
//  /* USER CODE END TIM2_Init 0 */
//
//  TIM_MasterConfigTypeDef sMasterConfig = {0};
//  TIM_IC_InitTypeDef sConfigIC = {0};
//
//  /* USER CODE BEGIN TIM2_Init 1 */
//
//  /* USER CODE END TIM2_Init 1 */
//  htim2.Instance = TIM2;
//  htim2.Init.Prescaler = 0;
//  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
//  htim2.Init.Period = 4294967295;
//  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
//  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
//  if (HAL_TIM_IC_Init(&htim2) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
//  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
//  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_BOTHEDGE;
//  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
//  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
//  sConfigIC.ICFilter = 0;
//  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  /* USER CODE BEGIN TIM2_Init 2 */
//
//  /* USER CODE END TIM2_Init 2 */
//
//}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

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

  /* USER CODE END USART2_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = { 0 };
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void UartClearScreen()
{
  printf("\033[2J");
  printf("\033[H"); // Move cursor to top-left corner
}

void UartSetCursorPosition(int row, int col)
{
  printf("\033[%d;%dH", row, col);
}

void SetCharAt(int row, int col, char *c)
{
  UartSetCursorPosition(row, col);
  printf(c);
}

void DrawRisingColumn(int col, int height)
{
  for (int r = 1; r < height; r++)
  {
    SetCharAt(r, col, "│");
  }
  SetCharAt(0, col, "┌");
  SetCharAt(height, col, "┘");
}

void DrawFallingColumn(int col, int height)
{
  for (int r = 1; r < height; r++)
  {
    SetCharAt(r, col, "│");
  }
  SetCharAt(0, col, "┐");
  SetCharAt(height, col, "└");
}

void DrawRow(int startCol, int endCol, int row)
{
  for (int c = startCol; c < endCol; c++)
  {
    SetCharAt(row, c, "─");
  }
}

static void RenderPwmUart(uint16_t duty, uint32_t freq)
{
  char msg[100];
  uint16_t width = 50;
  uint16_t height = 8;

  UartClearScreen();

  if (freq < 1000)
  {
    snprintf(msg, sizeof(msg), "No signal");
  } else
  {
    snprintf(msg, sizeof(msg), "Freq: %lu Hz, Duty: %d pct\r\n", freq, duty);
  }
  UartSetCursorPosition(10, 0);
  printf(msg);

  if (duty == 0)
  {
    DrawRow(0, 2 * width, height);
    return;
  }

  // first wave
  uint16_t secondCol = duty * width / 100;
  DrawRow(0, secondCol, 0);
  DrawRow(secondCol, width, height);
  DrawRisingColumn(0, height);
  DrawFallingColumn(secondCol, height);

  // second wave
  uint16_t thirdCol = secondCol + width;
  DrawRow(width, thirdCol, 0);
  DrawRow(thirdCol, width * 2, height);
  DrawRisingColumn(width, height);
  DrawFallingColumn(thirdCol, height);

  // start of third wave
  DrawRisingColumn(width * 2, height);

}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
    if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
    {
      // Rising edge
      last_rising = rising;
      rising = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
    } else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
    {
      // Falling edge
      falling = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
      new_data_available = 1;
    }
  }
}

int __io_putchar(int ch)
{
  if (HAL_UART_Transmit(&huart2, (uint8_t*) &ch, 1, HAL_MAX_DELAY) != HAL_OK)
  {
    return -1;
  }
  return ch;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    if (uart_rx_byte == '\r')
    {  // End of input (Enter key)
      uart_buffer[uart_index] = '\0';  // Null-terminate string
      uart_number_ready = true;
      uart_index = 0;
    } else if (uart_index < UART_BUFFER_SIZE - 1)
    {
      if (uart_rx_byte >= '0' && uart_rx_byte <= '9')
      {
        uart_buffer[uart_index++] = uart_rx_byte;
      }
    }

    // Continue receiving next character
    HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
  }
}

int ReadUARTNumber(void)
{
  if (uart_number_ready)
  {
    uart_number_ready = false;
    return atoi((char*) uart_buffer);  // Simple ASCII to int
  }
  return -1;  // Indicate no new number yet
}

// I don't know how to set this up in .IOC editor, so I hardcode it here. We cannot regenerate code now :-/
void MX_TIM2_Init(void)
{
  TIM_IC_InitTypeDef sConfigIC = { 0 };

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 0xFFFFFFFF;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  HAL_TIM_IC_Init(&htim2);

  // CH1 - Rising edge (period measurement)
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_1);

  // CH2 - Falling edge (high time measurement)
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
  sConfigIC.ICSelection = TIM_ICSELECTION_INDIRECTTI;
  HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_2);
}
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
