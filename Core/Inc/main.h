/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

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
#define KEY_3_Pin GPIO_PIN_2
#define KEY_3_GPIO_Port GPIOE
#define KEY_4_Pin GPIO_PIN_3
#define KEY_4_GPIO_Port GPIOE
#define KEY_1_Pin GPIO_PIN_4
#define KEY_1_GPIO_Port GPIOE
#define KEY_2_Pin GPIO_PIN_5
#define KEY_2_GPIO_Port GPIOE
#define MOTOR_AIN1_Pin GPIO_PIN_0
#define MOTOR_AIN1_GPIO_Port GPIOC
#define MOTOR_AIN2_Pin GPIO_PIN_1
#define MOTOR_AIN2_GPIO_Port GPIOC
#define MOTOR_BIN1_Pin GPIO_PIN_2
#define MOTOR_BIN1_GPIO_Port GPIOC
#define MOTOR_BIN2_Pin GPIO_PIN_3
#define MOTOR_BIN2_GPIO_Port GPIOC
#define MOTOR_STBY_Pin GPIO_PIN_4
#define MOTOR_STBY_GPIO_Port GPIOC
#define STEPPER_L_DIR_Pin GPIO_PIN_7
#define STEPPER_L_DIR_GPIO_Port GPIOE
#define STEPPER_L_EN_Pin GPIO_PIN_8
#define STEPPER_L_EN_GPIO_Port GPIOE
#define STEPPER_R_DIR_Pin GPIO_PIN_9
#define STEPPER_R_DIR_GPIO_Port GPIOE
#define STEPPER_R_EN_Pin GPIO_PIN_10
#define STEPPER_R_EN_GPIO_Port GPIOE
#define NRF_CSN_Pin GPIO_PIN_12
#define NRF_CSN_GPIO_Port GPIOB
#define NRF_CE_Pin GPIO_PIN_14
#define NRF_CE_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
