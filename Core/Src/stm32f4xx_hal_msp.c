/**
  ******************************************************************************
  * @file    Templates/Src/stm32f4xx_hal_msp.c
  * @author  MCD Application Team
  * @brief   HAL MSP module.
  *
  @verbatim
 ===============================================================================
                     ##### How to use this driver #####
 ===============================================================================
    [..]
    This file is generated automatically by STM32CubeMX and eventually modified
    by the user

  @endverbatim
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "bsp_config.h"
/** @addtogroup STM32F4xx_HAL_Driver
 * @{
 */

/** @defgroup HAL_MSP
 * @brief HAL MSP module.
 * @{
 */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/** @defgroup HAL_MSP_Private_Functions
 * @{
 */

/**
 * @brief  Initializes the Global MSP.
 * @param  None
 * @retval None
 */
void HAL_MspInit(void)
{
  /* NOTE : This function is generated automatically by STM32CubeMX and eventually
            modified by the user
   */

  /* Set NVIC priority group to Group 4 (4 bits for preemption, 0 bits for subpriority)
     Required for FreeRTOS to work correctly with Cortex-M NVIC */
  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
}

/**
 * @brief  DeInitializes the Global MSP.
 * @param  None
 * @retval None
 */
void HAL_MspDeInit(void)
{
  /* NOTE : This function is generated automatically by STM32CubeMX and eventually
            modified by the user
   */
}

/*===========================================================================*/
/*                               UART MSP                                     */
/*===========================================================================*/

/**
  * @brief UART MSP Initialization
  *        USART1 (debug) / USART2 (GNSS) / USART3 (LoRa) / USART6 (5G)
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
   GPIO_InitTypeDef gpio = {0};
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;

  /* -------- USART1: 调试串口 (CH340) -------- */
  if (huart->Instance == BSP_DBG_UART) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    gpio.Pin       = BSP_DBG_TX_PIN;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Alternate = BSP_DBG_TX_AF;
    HAL_GPIO_Init(BSP_DBG_TX_PORT, &gpio);

    gpio.Pin       = BSP_DBG_RX_PIN;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Alternate = BSP_DBG_RX_AF;
    HAL_GPIO_Init(BSP_DBG_RX_PORT, &gpio);
  }

  /* -------- USART2: GNSS ATGM336H -------- */
  if (huart->Instance == BSP_GNSS_UART) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    gpio.Pin       = BSP_GNSS_TX_PIN | BSP_GNSS_RX_PIN;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Alternate = BSP_GNSS_TX_AF;
    HAL_GPIO_Init(BSP_GNSS_TX_PORT, &gpio);
  }

  /* -------- USART3: LoRa E22-400T30D -------- */
  if (huart->Instance == BSP_LORA_UART) {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();

    gpio.Pin       = BSP_LORA_TX_PIN | BSP_LORA_RX_PIN;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Alternate = BSP_LORA_TX_AF;
    HAL_GPIO_Init(BSP_LORA_TX_PORT, &gpio);
  }

  /* -------- USART6: 5G-A RM500U -------- */
  if (huart->Instance == BSP_5G_UART) {
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_USART6_CLK_ENABLE();

    gpio.Pin       = BSP_5G_TX_PIN | BSP_5G_RX_PIN;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Alternate = BSP_5G_TX_AF;
    HAL_GPIO_Init(BSP_5G_TX_PORT, &gpio);
  }
}


/**
  * @brief UART MSP DeInit
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
  if (huart->Instance == BSP_DBG_UART) {
    __HAL_RCC_USART1_CLK_DISABLE();
    HAL_GPIO_DeInit(BSP_DBG_TX_PORT, BSP_DBG_TX_PIN | BSP_DBG_RX_PIN);
  }
  if (huart->Instance == BSP_GNSS_UART) {
    __HAL_RCC_USART2_CLK_DISABLE();
    HAL_GPIO_DeInit(BSP_GNSS_TX_PORT, BSP_GNSS_TX_PIN | BSP_GNSS_RX_PIN);
  }
  if (huart->Instance == BSP_LORA_UART) {
    __HAL_RCC_USART3_CLK_DISABLE();
    HAL_GPIO_DeInit(BSP_LORA_TX_PORT, BSP_LORA_TX_PIN | BSP_LORA_RX_PIN);
  }
  if (huart->Instance == BSP_5G_UART) {
    __HAL_RCC_USART6_CLK_DISABLE();
    HAL_GPIO_DeInit(BSP_5G_TX_PORT, BSP_5G_TX_PIN | BSP_5G_RX_PIN);
  }
}

/*===========================================================================*/
/*                               I2C MSP                                      */
/*===========================================================================*/

/**
  * @brief I2C1 MSP Init — MPU6050 + BME280
  */
void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance == BSP_I2C_INS) {
    GPIO_InitTypeDef gpio = {0};
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    gpio.Pin       = BSP_I2C_SCL_PIN | BSP_I2C_SDA_PIN;
    gpio.Mode      = GPIO_MODE_AF_OD;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Alternate = BSP_I2C_SCL_AF;
    HAL_GPIO_Init(BSP_I2C_SCL_PORT, &gpio);
  }
}

/**
  * @brief I2C1 MSP DeInit
  */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance == BSP_I2C_INS) {
    __HAL_RCC_I2C1_CLK_DISABLE();
    HAL_GPIO_DeInit(BSP_I2C_SCL_PORT, BSP_I2C_SCL_PIN | BSP_I2C_SDA_PIN);
  }
}

/*===========================================================================*/
/*                               CAN MSP                                      */
/*===========================================================================*/

/**
  * @brief CAN1 MSP Init — Remote ID (V1.1)
  */
void HAL_CAN_MspInit(CAN_HandleTypeDef *hcan)
{
  if (hcan->Instance == BSP_CAN_INS) {
    GPIO_InitTypeDef gpio = {0};
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_CAN1_CLK_ENABLE();

    gpio.Pin       = BSP_CAN_RX_PIN;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Alternate = BSP_CAN_RX_AF;
    HAL_GPIO_Init(BSP_CAN_RX_PORT, &gpio);

    gpio.Pin       = BSP_CAN_TX_PIN;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Alternate = BSP_CAN_TX_AF;
    HAL_GPIO_Init(BSP_CAN_TX_PORT, &gpio);
  }
}

/**
  * @brief CAN1 MSP DeInit
  */
void HAL_CAN_MspDeInit(CAN_HandleTypeDef *hcan)
{
  if (hcan->Instance == BSP_CAN_INS) {
    __HAL_RCC_CAN1_CLK_DISABLE();
    HAL_GPIO_DeInit(BSP_CAN_RX_PORT, BSP_CAN_RX_PIN | BSP_CAN_TX_PIN);
  }
}

/*===========================================================================*/
/*                              SDIO MSP                                      */
/*===========================================================================*/

/**
  * @brief SDIO MSP Init — MicroSD 4-bit 模式
  */
void HAL_SD_MspInit(SD_HandleTypeDef *hsd)
{
  if (hsd->Instance == BSP_SDIO_INS) {
    GPIO_InitTypeDef gpio = {0};
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_SDIO_CLK_ENABLE();

    /* D0~D3: 47K 上拉, 推挽 */
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Alternate = BSP_SDIO_D0_AF;

    gpio.Pin = BSP_SDIO_D0_PIN;
    HAL_GPIO_Init(BSP_SDIO_D0_PORT, &gpio);
    gpio.Pin = BSP_SDIO_D1_PIN;
    HAL_GPIO_Init(BSP_SDIO_D1_PORT, &gpio);
    gpio.Pin = BSP_SDIO_D2_PIN;
    HAL_GPIO_Init(BSP_SDIO_D2_PORT, &gpio);
    gpio.Pin = BSP_SDIO_D3_PIN;
    HAL_GPIO_Init(BSP_SDIO_D3_PORT, &gpio);

    /* CK: 推挽, 无上拉 */
    gpio.Pull = GPIO_NOPULL;
    gpio.Pin  = BSP_SDIO_CK_PIN;
    HAL_GPIO_Init(BSP_SDIO_CK_PORT, &gpio);

    /* CMD: 47K 上拉 */
    gpio.Pull = GPIO_PULLUP;
    gpio.Pin  = BSP_SDIO_CMD_PIN;
    HAL_GPIO_Init(BSP_SDIO_CMD_PORT, &gpio);
  }
}

/**
  * @brief SDIO MSP DeInit
  */
void HAL_SD_MspDeInit(SD_HandleTypeDef *hsd)
{
  if (hsd->Instance == BSP_SDIO_INS) {
    __HAL_RCC_SDIO_CLK_DISABLE();
    HAL_GPIO_DeInit(BSP_SDIO_D0_PORT, BSP_SDIO_D0_PIN | BSP_SDIO_D1_PIN |
                    BSP_SDIO_D2_PIN | BSP_SDIO_D3_PIN | BSP_SDIO_CK_PIN);
    HAL_GPIO_DeInit(BSP_SDIO_CMD_PORT, BSP_SDIO_CMD_PIN);
  }
}

/*===========================================================================*/
/*                              TIM MSP                                       */
/*===========================================================================*/

/**
  * @brief TIM Base MSP Init — TIM3/4 (电机) + TIM13 (蜂鸣器)
  */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
  GPIO_InitTypeDef gpio = {0};
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio.Mode  = GPIO_MODE_AF_PP;
  gpio.Pull  = GPIO_NOPULL;

  /* -------- TIM3 CH1(CH2): 电机1/2 PWM -------- */
  if (htim->Instance == BSP_PWM_MOTOR1_TIM) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();

    gpio.Alternate = BSP_PWM_MOTOR1_AF;
    gpio.Pin       = BSP_PWM_MOTOR1_PIN;
    HAL_GPIO_Init(BSP_PWM_MOTOR1_PORT, &gpio);

    gpio.Pin       = BSP_PWM_MOTOR2_PIN;
    HAL_GPIO_Init(BSP_PWM_MOTOR2_PORT, &gpio);
  }

  /* -------- TIM4 CH1(CH2): 电机3/4 PWM -------- */
  if (htim->Instance == BSP_PWM_MOTOR3_TIM) {
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();

    gpio.Alternate = BSP_PWM_MOTOR3_AF;
    gpio.Pin       = BSP_PWM_MOTOR3_PIN;
    HAL_GPIO_Init(BSP_PWM_MOTOR3_PORT, &gpio);

    gpio.Pin       = BSP_PWM_MOTOR4_PIN;
    HAL_GPIO_Init(BSP_PWM_MOTOR4_PORT, &gpio);
  }

  /* -------- TIM13 CH1: 蜂鸣器 PWM -------- */
  if (htim->Instance == BSP_BEEP_TIM) {
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_TIM13_CLK_ENABLE();

    gpio.Alternate = BSP_BEEP_AF;
    gpio.Pin       = BSP_BEEP_PIN;
    HAL_GPIO_Init(BSP_BEEP_PORT, &gpio);
  }
}

/**
  * @brief TIM Base MSP DeInit
  */
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == BSP_PWM_MOTOR1_TIM) {
    __HAL_RCC_TIM3_CLK_DISABLE();
    HAL_GPIO_DeInit(BSP_PWM_MOTOR1_PORT, BSP_PWM_MOTOR1_PIN | BSP_PWM_MOTOR2_PIN);
  }
  if (htim->Instance == BSP_PWM_MOTOR3_TIM) {
    __HAL_RCC_TIM4_CLK_DISABLE();
    HAL_GPIO_DeInit(BSP_PWM_MOTOR3_PORT, BSP_PWM_MOTOR3_PIN | BSP_PWM_MOTOR4_PIN);
  }
  if (htim->Instance == BSP_BEEP_TIM) {
    __HAL_RCC_TIM13_CLK_DISABLE();
    HAL_GPIO_DeInit(BSP_BEEP_PORT, BSP_BEEP_PIN);
  }
}

/*===========================================================================*/
/*                              ADC MSP                                       */
/*===========================================================================*/

/**
  * @brief ADC1 MSP Init — 电源采样
  */
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
  if (hadc->Instance == BSP_ADC_INS) {
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();

    gpio.Pin   = BSP_ADC_PIN;
    gpio.Mode  = GPIO_MODE_ANALOG;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BSP_ADC_PORT, &gpio);
  }
}

/**
  * @brief ADC1 MSP DeInit
  */
void HAL_ADC_MspDeInit(ADC_HandleTypeDef *hadc)
{
  if (hadc->Instance == BSP_ADC_INS) {
    __HAL_RCC_ADC1_CLK_DISABLE();
    HAL_GPIO_DeInit(BSP_ADC_PORT, BSP_ADC_PIN);
  }
}
/**
 * @}
 */

/**
 * @}
 */
