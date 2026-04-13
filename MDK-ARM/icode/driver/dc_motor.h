#ifndef __DC_MOTOR_H
#define __DC_MOTOR_H

#include "main.h"
#include <stdint.h>

/*======================= 用户可调参数 =======================*/
/* 车轮直径，单位：米（默认 6.5cm） */
#define DCMOTOR_WHEEL_DIAMETER_M      (0.065f)
/* 编码器每圈计数（4倍频后的总计数），请按你的电机实测修改 */
#define DCMOTOR_ENCODER_CPR           (880.0f)

/* 编码器方向修正（接线相反时改成 1） */
#define DCMOTOR_LEFT_ENCODER_REVERSE  (0)
#define DCMOTOR_RIGHT_ENCODER_REVERSE (1)
/* 电机方向修正（右轮朝向相反时改成 1） */

/*===========================================================*/

typedef enum
{
    DCMOTOR_DIR_STOP = 0,
    DCMOTOR_DIR_FORWARD,
    DCMOTOR_DIR_BACKWARD
} DCMotor_Direction_t;

typedef struct
{
    DCMotor_Direction_t left_dir;
    DCMotor_Direction_t right_dir;
    int16_t left_duty_percent;   /* -100 ~ 100 */
    int16_t right_duty_percent;  /* -100 ~ 100 */
    float left_speed_mps;        /* 左轮线速度，m/s（带符号） */
    float right_speed_mps;       /* 右轮线速度，m/s（带符号） */
    int32_t left_delta_count;    /* 本周期编码器增量 */
    int32_t right_delta_count;   /* 本周期编码器增量 */
} DCMotor_Status_t;

void DCMotor_Init(void);
void DCMotor_Enable(uint8_t enable);
void DCMotor_SetDuty(int16_t left_duty_percent, int16_t right_duty_percent);
void DCMotor_UpdateSpeed(float sample_time_s);
void DCMotor_GetStatus(DCMotor_Status_t *status);
const char *DCMotor_DirectionString(DCMotor_Direction_t dir);

#endif
