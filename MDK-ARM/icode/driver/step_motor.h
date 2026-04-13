#ifndef __STEP_MOTOR_H
#define __STEP_MOTOR_H

#include "main.h"
#include <stdint.h>

/*
 * DRV8825 说明：
 * - DIR：方向控制
 * - EN ：使能（通常为低有效，若你的板子是高有效，请改成 0）
 * - STEP：PWM 输入（由 TIM9 CH1/CH2 输出）
 */
#define STEPPER_EN_ACTIVE_LOW   (1)

typedef enum
{
    STEPPER_LEFT = 0,
    STEPPER_RIGHT = 1
} StepperId_t;

typedef enum
{
    STEPPER_DIR_CW = 0,
    STEPPER_DIR_CCW = 1
} StepperDir_t;

/* 步进电机参数：步距角 + 细分数 */
#define STEPPER_STEP_ANGLE_DEG   (1.8f)
#define STEPPER_MICROSTEP        (32U)

/* 每转一圈需要的脉冲数 = 360 / 步距角 * 细分 */
#define STEPPER_PULSES_PER_REV \
    ((uint32_t)((360.0f / STEPPER_STEP_ANGLE_DEG) * (float)STEPPER_MICROSTEP))

void Stepper_Init(void);
void Stepper_SetEnable(StepperId_t id, uint8_t enable);
void Stepper_SetDir(StepperId_t id, StepperDir_t dir);

/* 设置步进脉冲频率（Hz），左右独立 */
void Stepper_SetStepFreq(StepperId_t id, uint32_t step_hz);

/* 设置转速（RPM），内部换算成步进频率 */
void Stepper_SetSpeedRpm(StepperId_t id, float rpm);

#endif
