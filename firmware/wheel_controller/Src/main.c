/*
 * PETSim-Racing - wheel_controller
 *
 * Smoke-test firmware for the STM32 <-> ODrive UART link (see
 * docs/02_stm32_odrive_wiring.md). Periodically queries the encoder position
 * over USART1 (ODrive ASCII protocol) and blinks the onboard LED so you can
 * see it's alive even without a serial terminal attached.
 */
#include "main.h"
#include <string.h>

UART_HandleTypeDef huart1;

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void ODrive_SendCommand(const char *cmd);
static uint16_t ODrive_ReadLine(char *buf, uint16_t buf_size, uint32_t timeout_ms);

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART1_UART_Init();

  char rx_line[64];

  while (1)
  {
    ODrive_SendCommand("r axis0.encoder.pos_estimate\n");
    uint16_t len = ODrive_ReadLine(rx_line, sizeof(rx_line), 200);
    (void)len; /* rx_line now holds the ODrive's reply (or is empty on timeout) */

    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    HAL_Delay(500);
  }
}

static void ODrive_SendCommand(const char *cmd)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)cmd, (uint16_t)strlen(cmd), 100);
}

/* Reads bytes until '\n' or timeout. Returns number of bytes read (excluding
 * the null terminator). buf is always null-terminated. */
static uint16_t ODrive_ReadLine(char *buf, uint16_t buf_size, uint32_t timeout_ms)
{
  uint16_t idx = 0;
  uint32_t start = HAL_GetTick();

  while ((HAL_GetTick() - start) < timeout_ms && idx < (buf_size - 1))
  {
    uint8_t byte;
    if (HAL_UART_Receive(&huart1, &byte, 1, 20) == HAL_OK)
    {
      if (byte == '\n')
      {
        break;
      }
      buf[idx++] = (char)byte;
    }
  }
  buf[idx] = '\0';
  return idx;
}

/**
  * SYSCLK = 96 MHz, from HSE 25 MHz.
  * PLLQ is set to 48 MHz so USB OTG FS can reuse this clock config later.
  */
static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                               | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();

  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET); /* LED off (active low) */

  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);
}

static void MX_USART1_UART_Init(void)
{
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
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    for (volatile uint32_t i = 0; i < 200000; i++) {}
  }
}
