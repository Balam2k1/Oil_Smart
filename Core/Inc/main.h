/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define DS_Pin GPIO_PIN_13
#define DS_GPIO_Port GPIOC
#define ST_CP_Pin GPIO_PIN_14
#define ST_CP_GPIO_Port GPIOC
#define SH_CP_Pin GPIO_PIN_15
#define SH_CP_GPIO_Port GPIOC
#define LED_GPS_Pin GPIO_PIN_0
#define LED_GPS_GPIO_Port GPIOF
#define LED_OLDC_Pin GPIO_PIN_1
#define LED_OLDC_GPIO_Port GPIOF
#define ADC_Vin_Pin GPIO_PIN_1
#define ADC_Vin_GPIO_Port GPIOC
#define CLK_LED1_Pin GPIO_PIN_2
#define CLK_LED1_GPIO_Port GPIOC
#define LOAD_DATA1_Pin GPIO_PIN_3
#define LOAD_DATA1_GPIO_Port GPIOC
#define DATA_LED1_Pin GPIO_PIN_0
#define DATA_LED1_GPIO_Port GPIOA
#define LED_TEMP_C_Pin GPIO_PIN_1
#define LED_TEMP_C_GPIO_Port GPIOA
#define TX2_Pin GPIO_PIN_2
#define TX2_GPIO_Port GPIOA
#define RX2_Pin GPIO_PIN_3
#define RX2_GPIO_Port GPIOA
#define DIR_485_Pin GPIO_PIN_4
#define DIR_485_GPIO_Port GPIOA
#define RELAY_1_Pin GPIO_PIN_5
#define RELAY_1_GPIO_Port GPIOA
#define RELAY_2_Pin GPIO_PIN_6
#define RELAY_2_GPIO_Port GPIOA
#define TEMP_Pin GPIO_PIN_7
#define TEMP_GPIO_Port GPIOA
#define PA_Pin GPIO_PIN_4
#define PA_GPIO_Port GPIOC
#define OIL_1_Pin GPIO_PIN_5
#define OIL_1_GPIO_Port GPIOC
#define OIL_1_EXTI_IRQn EXTI4_15_IRQn
#define TACHO_Pin GPIO_PIN_0
#define TACHO_GPIO_Port GPIOB
#define OIL_2_Pin GPIO_PIN_1
#define OIL_2_GPIO_Port GPIOB
#define OIL_2_EXTI_IRQn EXTI0_1_IRQn
#define TX3_Pin GPIO_PIN_10
#define TX3_GPIO_Port GPIOB
#define RX3_Pin GPIO_PIN_11
#define RX3_GPIO_Port GPIOB
#define OUT3_Pin GPIO_PIN_12
#define OUT3_GPIO_Port GPIOB
#define OUT2_Pin GPIO_PIN_13
#define OUT2_GPIO_Port GPIOB
#define OUT1_Pin GPIO_PIN_14
#define OUT1_GPIO_Port GPIOB
#define DATA_LED2_Pin GPIO_PIN_15
#define DATA_LED2_GPIO_Port GPIOB
#define LED_PA_C_Pin GPIO_PIN_6
#define LED_PA_C_GPIO_Port GPIOC
#define FL_CS_Pin GPIO_PIN_7
#define FL_CS_GPIO_Port GPIOC
#define FL_SO_Pin GPIO_PIN_8
#define FL_SO_GPIO_Port GPIOC
#define FL_SI_Pin GPIO_PIN_9
#define FL_SI_GPIO_Port GPIOC
#define FL_SCK_Pin GPIO_PIN_8
#define FL_SCK_GPIO_Port GPIOA
#define TX1_Pin GPIO_PIN_9
#define TX1_GPIO_Port GPIOA
#define RX1_Pin GPIO_PIN_10
#define RX1_GPIO_Port GPIOA
#define RESET_ESSP_Pin GPIO_PIN_15
#define RESET_ESSP_GPIO_Port GPIOA
#define LOAD_DATA2_Pin GPIO_PIN_12
#define LOAD_DATA2_GPIO_Port GPIOC
#define CLK_LED2_Pin GPIO_PIN_2
#define CLK_LED2_GPIO_Port GPIOD
#define LOAD_DATA_Pin GPIO_PIN_3
#define LOAD_DATA_GPIO_Port GPIOB
#define DATA_LED_Pin GPIO_PIN_4
#define DATA_LED_GPIO_Port GPIOB
#define LOAD_DATAB5_Pin GPIO_PIN_5
#define LOAD_DATAB5_GPIO_Port GPIOB
#define LED_TACHO_Pin GPIO_PIN_6
#define LED_TACHO_GPIO_Port GPIOB
#define LED_VIN_Pin GPIO_PIN_7
#define LED_VIN_GPIO_Port GPIOB
#define BUZZER_Pin GPIO_PIN_9
#define BUZZER_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
