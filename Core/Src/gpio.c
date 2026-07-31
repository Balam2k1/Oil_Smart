/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, DS_Pin|ST_CP_Pin|SH_CP_Pin|CLK_LED1_Pin
                          |LOAD_DATA1_Pin|LED_PA_C_Pin|FL_CS_Pin|FL_SI_Pin
                          |LOAD_DATA2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, LED_GPS_Pin|LED_OLDC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, DATA_LED1_Pin|LED_TEMP_C_Pin|DIR_485_Pin|RELAY_1_Pin
                          |RELAY_2_Pin|FL_SCK_Pin|RESET_ESSP_Pin, GPIO_PIN_RESET);

  HAL_GPIO_WritePin(GPIOA,RESET_ESSP_Pin, SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, OUT3_Pin|OUT2_Pin|OUT1_Pin|DATA_LED2_Pin
                          |LOAD_DATA_Pin|DATA_LED_Pin|LOAD_DATAB5_Pin|LED_TACHO_Pin
                          |LED_VIN_Pin|BUZZER_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CLK_LED2_GPIO_Port, CLK_LED2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : DS_Pin ST_CP_Pin SH_CP_Pin CLK_LED1_Pin
                           LOAD_DATA1_Pin LED_PA_C_Pin FL_CS_Pin FL_SI_Pin
                           LOAD_DATA2_Pin */
  GPIO_InitStruct.Pin = DS_Pin|ST_CP_Pin|SH_CP_Pin|CLK_LED1_Pin
                          |LOAD_DATA1_Pin|LED_PA_C_Pin|FL_CS_Pin|FL_SI_Pin
                          |LOAD_DATA2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_GPS_Pin LED_OLDC_Pin */
  GPIO_InitStruct.Pin = LED_GPS_Pin|LED_OLDC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : DATA_LED1_Pin LED_TEMP_C_Pin DIR_485_Pin RELAY_1_Pin
                           RELAY_2_Pin FL_SCK_Pin RESET_ESSP_Pin */
  GPIO_InitStruct.Pin = DATA_LED1_Pin|LED_TEMP_C_Pin|DIR_485_Pin|RELAY_1_Pin
                          |RELAY_2_Pin|FL_SCK_Pin|RESET_ESSP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : OIL_1_Pin */
  GPIO_InitStruct.Pin = OIL_1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OIL_1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : OIL_2_Pin */
  GPIO_InitStruct.Pin = OIL_2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OIL_2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : OUT3_Pin OUT2_Pin OUT1_Pin DATA_LED2_Pin
                           LOAD_DATA_Pin DATA_LED_Pin LOAD_DATAB5_Pin LED_TACHO_Pin
                           LED_VIN_Pin BUZZER_Pin */
  GPIO_InitStruct.Pin = OUT3_Pin|OUT2_Pin|OUT1_Pin|DATA_LED2_Pin
                          |LOAD_DATA_Pin|DATA_LED_Pin|LOAD_DATAB5_Pin|LED_TACHO_Pin
                          |LED_VIN_Pin|BUZZER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : FL_SO_Pin */
  GPIO_InitStruct.Pin = FL_SO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(FL_SO_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : CLK_LED2_Pin */
  GPIO_InitStruct.Pin = CLK_LED2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CLK_LED2_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
