#ifndef BSP_CONFIG_H
#define BSP_CONFIG_H

/*******************************************************************************
 * @file    bsp_config.h
 * @brief   引脚宏定义 — 严格对照 Docs/02_完整引脚分配表.md (62 引脚)
 * @note    移植时只改此文件。不加表外引脚。
 * @hw      STM32F407ZGTx, LQFP-144, 168MHz
 *
 * 关键设计决策：
 *   PA0 硬件固定 WK_UP → LoRa 无法用 UART4 → 改用 USART3 PB10/PB11
 *   CAN1 经 P5 跳线用 PA11/PA12 → 放弃 USB OTG
 *   禁用摄像头(DCMI) → 释放 PA8/PE5/PE6/PB6/PB7
 *   禁用音频(ES8388) → 释放 PC6/PC7
 *   以太网 PHY 保持复位 → 释放 PA1/PA7/PC4/PC5/PG11/PG13/PG14
 ******************************************************************************/

/*===========================================================================
 * 1. 串口与 CAN 接口 (12 pin)
 *===========================================================================*/

/* --- 调试 USART1 (CH340, P10 跳线→USB) --- */
#define BSP_DBG_UART              USART1
#define BSP_DBG_UART_BAUD         115200
#define BSP_DBG_UART_WORD         UART_WORDLENGTH_8B
#define BSP_DBG_UART_STOP         UART_STOPBITS_1
#define BSP_DBG_UART_PARITY       UART_PARITY_NONE
#define BSP_DBG_UART_MODE         UART_MODE_TX_RX
#define BSP_DBG_UART_HWCTL        UART_HWCONTROL_NONE
#define BSP_DBG_UART_OVERSAMP     UART_OVERSAMPLING_16

#define BSP_DBG_TX_PORT           GPIOA
#define BSP_DBG_TX_PIN            GPIO_PIN_9
#define BSP_DBG_TX_AF             GPIO_AF7_USART1

#define BSP_DBG_RX_PORT           GPIOA
#define BSP_DBG_RX_PIN            GPIO_PIN_10
#define BSP_DBG_RX_AF             GPIO_AF7_USART1

/* --- GNSS ATGM336H — USART2 (NMEA 输出) --- */
#define BSP_GNSS_UART             USART2
#define BSP_GNSS_UART_BAUD        9600
#define BSP_GNSS_UART_WORD        UART_WORDLENGTH_8B
#define BSP_GNSS_UART_STOP        UART_STOPBITS_1
#define BSP_GNSS_UART_PARITY      UART_PARITY_NONE
#define BSP_GNSS_UART_MODE        UART_MODE_TX_RX
#define BSP_GNSS_UART_HWCTL       UART_HWCONTROL_NONE
#define BSP_GNSS_UART_OVERSAMP    UART_OVERSAMPLING_16

#define BSP_GNSS_TX_PORT          GPIOA
#define BSP_GNSS_TX_PIN           GPIO_PIN_2
#define BSP_GNSS_TX_AF            GPIO_AF7_USART2

#define BSP_GNSS_RX_PORT          GPIOA
#define BSP_GNSS_RX_PIN           GPIO_PIN_3
#define BSP_GNSS_RX_AF            GPIO_AF7_USART2

/* --- LoRa E22-400T30D — USART3 (原 UART4 PA0 被 WK_UP 占用) --- */
#define BSP_LORA_UART             USART3
#define BSP_LORA_UART_BAUD        9600
#define BSP_LORA_UART_WORD        UART_WORDLENGTH_8B
#define BSP_LORA_UART_STOP        UART_STOPBITS_1
#define BSP_LORA_UART_PARITY      UART_PARITY_NONE
#define BSP_LORA_UART_MODE        UART_MODE_TX_RX
#define BSP_LORA_UART_HWCTL       UART_HWCONTROL_NONE
#define BSP_LORA_UART_OVERSAMP    UART_OVERSAMPLING_16

#define BSP_LORA_TX_PORT          GPIOB
#define BSP_LORA_TX_PIN           GPIO_PIN_10
#define BSP_LORA_TX_AF            GPIO_AF7_USART3

#define BSP_LORA_RX_PORT          GPIOB
#define BSP_LORA_RX_PIN           GPIO_PIN_11
#define BSP_LORA_RX_AF            GPIO_AF7_USART3

/* --- 5G-A RM500U — USART6 (AT 指令) --- */
#define BSP_5G_UART               USART6
#define BSP_5G_UART_BAUD          115200
#define BSP_5G_UART_WORD          UART_WORDLENGTH_8B
#define BSP_5G_UART_STOP          UART_STOPBITS_1
#define BSP_5G_UART_PARITY        UART_PARITY_NONE
#define BSP_5G_UART_MODE          UART_MODE_TX_RX
#define BSP_5G_UART_HWCTL         UART_HWCONTROL_NONE
#define BSP_5G_UART_OVERSAMP      UART_OVERSAMPLING_16

#define BSP_5G_TX_PORT            GPIOC
#define BSP_5G_TX_PIN             GPIO_PIN_6
#define BSP_5G_TX_AF              GPIO_AF8_USART6

#define BSP_5G_RX_PORT            GPIOC
#define BSP_5G_RX_PIN             GPIO_PIN_7
#define BSP_5G_RX_AF              GPIO_AF8_USART6

/* --- Remote ID (V1.1) — CAN1 (P5 跳线, 放弃 USB OTG) --- */
#define BSP_CAN_INS               CAN1
#define BSP_CAN_BAUD              500000

#define BSP_CAN_RX_PORT           GPIOA
#define BSP_CAN_RX_PIN            GPIO_PIN_11
#define BSP_CAN_RX_AF             GPIO_AF9_CAN1

#define BSP_CAN_TX_PORT           GPIOA
#define BSP_CAN_TX_PIN            GPIO_PIN_12
#define BSP_CAN_TX_AF             GPIO_AF9_CAN1

/* --- SWD 调试 (ST-Link) --- */
/* PA13=SWDIO PA14=SWCLK 硬件固定, 不需要宏 */

/*===========================================================================
 * 2. SDIO — MicroSD 卡 (6 pin)
 *===========================================================================*/

#define BSP_SDIO_INS              SDIO
#define BSP_SDIO_CLK_DIV          0x76U

#define BSP_SDIO_D0_PORT          GPIOC
#define BSP_SDIO_D0_PIN           GPIO_PIN_8
#define BSP_SDIO_D0_AF            GPIO_AF12_SDIO

#define BSP_SDIO_D1_PORT          GPIOC
#define BSP_SDIO_D1_PIN           GPIO_PIN_9
#define BSP_SDIO_D1_AF            GPIO_AF12_SDIO

#define BSP_SDIO_D2_PORT          GPIOC
#define BSP_SDIO_D2_PIN           GPIO_PIN_10
#define BSP_SDIO_D2_AF            GPIO_AF12_SDIO

#define BSP_SDIO_D3_PORT          GPIOC
#define BSP_SDIO_D3_PIN           GPIO_PIN_11
#define BSP_SDIO_D3_AF            GPIO_AF12_SDIO

#define BSP_SDIO_CK_PORT          GPIOC
#define BSP_SDIO_CK_PIN           GPIO_PIN_12
#define BSP_SDIO_CK_AF            GPIO_AF12_SDIO

#define BSP_SDIO_CMD_PORT         GPIOD
#define BSP_SDIO_CMD_PIN          GPIO_PIN_2
#define BSP_SDIO_CMD_AF           GPIO_AF12_SDIO

/*===========================================================================
 * 3. I2C1 — MPU6050 + BME280 (2 pin)
 *===========================================================================*/

#define BSP_I2C_INS               I2C1
#define BSP_I2C_SPEED             400000

#define BSP_I2C_SCL_PORT          GPIOB
#define BSP_I2C_SCL_PIN           GPIO_PIN_6
#define BSP_I2C_SCL_AF            GPIO_AF4_I2C1

#define BSP_I2C_SDA_PORT          GPIOB
#define BSP_I2C_SDA_PIN           GPIO_PIN_7
#define BSP_I2C_SDA_AF            GPIO_AF4_I2C1

/*===========================================================================
 * 4. 电机 PWM — TIM3 + TIM4 (4 pin)
 *===========================================================================*/

#define BSP_PWM_FREQ              1000U

#define BSP_PWM_MOTOR1_TIM        TIM3
#define BSP_PWM_MOTOR1_CH         TIM_CHANNEL_1
#define BSP_PWM_MOTOR1_PORT       GPIOA
#define BSP_PWM_MOTOR1_PIN        GPIO_PIN_6
#define BSP_PWM_MOTOR1_AF         GPIO_AF2_TIM3

#define BSP_PWM_MOTOR2_TIM        TIM3
#define BSP_PWM_MOTOR2_CH         TIM_CHANNEL_2
#define BSP_PWM_MOTOR2_PORT       GPIOA
#define BSP_PWM_MOTOR2_PIN        GPIO_PIN_7
#define BSP_PWM_MOTOR2_AF         GPIO_AF2_TIM3

#define BSP_PWM_MOTOR3_TIM        TIM4
#define BSP_PWM_MOTOR3_CH         TIM_CHANNEL_1
#define BSP_PWM_MOTOR3_PORT       GPIOD
#define BSP_PWM_MOTOR3_PIN        GPIO_PIN_12
#define BSP_PWM_MOTOR3_AF         GPIO_AF2_TIM4

#define BSP_PWM_MOTOR4_TIM        TIM4
#define BSP_PWM_MOTOR4_CH         TIM_CHANNEL_2
#define BSP_PWM_MOTOR4_PORT       GPIOD
#define BSP_PWM_MOTOR4_PIN        GPIO_PIN_13
#define BSP_PWM_MOTOR4_AF         GPIO_AF2_TIM4

/*===========================================================================
 * 5. 电机方向 GPIO (4 pin)
 *===========================================================================*/

#define BSP_MOTOR_DIR1_PORT       GPIOA
#define BSP_MOTOR_DIR1_PIN        GPIO_PIN_8

#define BSP_MOTOR_DIR2_PORT       GPIOE
#define BSP_MOTOR_DIR2_PIN        GPIO_PIN_5

#define BSP_MOTOR_DIR3_PORT       GPIOE
#define BSP_MOTOR_DIR3_PIN        GPIO_PIN_6

#define BSP_MOTOR_DIR4_PORT       GPIOA
#define BSP_MOTOR_DIR4_PIN        GPIO_PIN_4

/*===========================================================================
 * 6. LoRa 控制 + 按键 (7 pin)
 *===========================================================================*/

/* LoRa E22 模式控制 */
#define BSP_LORA_AUX_PORT         GPIOF
#define BSP_LORA_AUX_PIN          GPIO_PIN_0

#define BSP_LORA_M0_PORT          GPIOF
#define BSP_LORA_M0_PIN           GPIO_PIN_1

#define BSP_LORA_M1_PORT          GPIOF
#define BSP_LORA_M1_PIN           GPIO_PIN_2

/* 按键 (板载固定) */
#define BSP_BTN_WKUP_PORT         GPIOA
#define BSP_BTN_WKUP_PIN          GPIO_PIN_0

#define BSP_BTN_KEY0_PORT         GPIOE
#define BSP_BTN_KEY0_PIN          GPIO_PIN_4

#define BSP_BTN_KEY1_PORT         GPIOE
#define BSP_BTN_KEY1_PIN          GPIO_PIN_3

#define BSP_BTN_KEY2_PORT         GPIOE
#define BSP_BTN_KEY2_PIN          GPIO_PIN_2

/*===========================================================================
 * 7. FSMC 并口 LCD ATK-MD0700 (20 pin)
 *===========================================================================*/

/* 控制线 */
#define BSP_LCD_FSMC_CS_PORT      GPIOG
#define BSP_LCD_FSMC_CS_PIN       GPIO_PIN_12
#define BSP_LCD_FSMC_CS_AF        GPIO_AF12_FSMC

#define BSP_LCD_FSMC_RS_PORT      GPIOF
#define BSP_LCD_FSMC_RS_PIN       GPIO_PIN_12
#define BSP_LCD_FSMC_RS_AF        GPIO_AF12_FSMC

#define BSP_LCD_FSMC_RD_PORT      GPIOD
#define BSP_LCD_FSMC_RD_PIN       GPIO_PIN_4
#define BSP_LCD_FSMC_RD_AF        GPIO_AF12_FSMC

#define BSP_LCD_FSMC_WR_PORT      GPIOD
#define BSP_LCD_FSMC_WR_PIN       GPIO_PIN_5
#define BSP_LCD_FSMC_WR_AF        GPIO_AF12_FSMC

/* 16-bit 数据总线 */
#define BSP_LCD_FSMC_D0_PORT      GPIOD
#define BSP_LCD_FSMC_D0_PIN       GPIO_PIN_14

#define BSP_LCD_FSMC_D1_PORT      GPIOD
#define BSP_LCD_FSMC_D1_PIN       GPIO_PIN_15

#define BSP_LCD_FSMC_D2_PORT      GPIOD
#define BSP_LCD_FSMC_D2_PIN       GPIO_PIN_0

#define BSP_LCD_FSMC_D3_PORT      GPIOD
#define BSP_LCD_FSMC_D3_PIN       GPIO_PIN_1

#define BSP_LCD_FSMC_D4_PORT      GPIOE
#define BSP_LCD_FSMC_D4_PIN       GPIO_PIN_7

#define BSP_LCD_FSMC_D5_PORT      GPIOE
#define BSP_LCD_FSMC_D5_PIN       GPIO_PIN_8

#define BSP_LCD_FSMC_D6_PORT      GPIOE
#define BSP_LCD_FSMC_D6_PIN       GPIO_PIN_9

#define BSP_LCD_FSMC_D7_PORT      GPIOE
#define BSP_LCD_FSMC_D7_PIN       GPIO_PIN_10

#define BSP_LCD_FSMC_D8_PORT      GPIOE
#define BSP_LCD_FSMC_D8_PIN       GPIO_PIN_11

#define BSP_LCD_FSMC_D9_PORT      GPIOE
#define BSP_LCD_FSMC_D9_PIN       GPIO_PIN_12

#define BSP_LCD_FSMC_D10_PORT     GPIOE
#define BSP_LCD_FSMC_D10_PIN      GPIO_PIN_13

#define BSP_LCD_FSMC_D11_PORT     GPIOE
#define BSP_LCD_FSMC_D11_PIN      GPIO_PIN_14

#define BSP_LCD_FSMC_D12_PORT     GPIOE
#define BSP_LCD_FSMC_D12_PIN      GPIO_PIN_15

#define BSP_LCD_FSMC_D13_PORT     GPIOD
#define BSP_LCD_FSMC_D13_PIN      GPIO_PIN_8

#define BSP_LCD_FSMC_D14_PORT     GPIOD
#define BSP_LCD_FSMC_D14_PIN      GPIO_PIN_9

#define BSP_LCD_FSMC_D15_PORT     GPIOD
#define BSP_LCD_FSMC_D15_PIN      GPIO_PIN_10

/*===========================================================================
 * 8. 触摸 GT911 I2C — TFTLCD 插座直连 (4 pin)
 *===========================================================================*/

#define BSP_TOUCH_SCL_PORT        GPIOB
#define BSP_TOUCH_SCL_PIN         GPIO_PIN_0

#define BSP_TOUCH_SDA_PORT        GPIOF
#define BSP_TOUCH_SDA_PIN         GPIO_PIN_11

#define BSP_TOUCH_INT_PORT        GPIOB
#define BSP_TOUCH_INT_PIN         GPIO_PIN_1

#define BSP_TOUCH_RST_PORT        GPIOC
#define BSP_TOUCH_RST_PIN         GPIO_PIN_13

/*===========================================================================
 * 9. LCD 背光 — PB15 PWM (1 pin)
 *===========================================================================*/

#define BSP_LCD_BL_PORT           GPIOB
#define BSP_LCD_BL_PIN            GPIO_PIN_15

/*===========================================================================
 * 10. 蜂鸣器 — TIM13_CH1 PWM (1 pin)
 *     注: 分配表写 AF3, F407 硬件实际为 AF9, GPIO_AF3_TIM13 不存在
 *===========================================================================*/

#define BSP_BEEP_TIM              TIM13
#define BSP_BEEP_CH               TIM_CHANNEL_1
#define BSP_BEEP_PORT             GPIOF
#define BSP_BEEP_PIN              GPIO_PIN_8
#define BSP_BEEP_AF               GPIO_AF9_TIM13
#define BSP_BEEP_FREQ             4000U

/*===========================================================================
 * 11. 电源采样 — ADC1_IN5 (1 pin)
 *===========================================================================*/

#define BSP_ADC_INS               ADC1
#define BSP_ADC_CH                ADC_CHANNEL_5
#define BSP_ADC_PORT              GPIOA
#define BSP_ADC_PIN               GPIO_PIN_5

#endif /* BSP_CONFIG_H */
