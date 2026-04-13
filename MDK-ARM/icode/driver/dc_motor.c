#include "dc_motor.h"
#include "tim.h"
#include <math.h>

/* 本工程使用：TIM4_CH1/CH2 -> PWMA/PWMB，TIM2/TIM3 -> 编码器 */

static DCMotor_Status_t s_motor = {0};

static int16_t s_last_cnt_left = 0;
static int16_t s_last_cnt_right = 0;

static int16_t DCMotor_ClampDuty(int16_t duty)
{
    if (duty > 100) return 100;
    if (duty < -100) return -100;
    return duty;
}

static void DCMotor_SetLeftDirection(DCMotor_Direction_t dir)
{
    switch (dir)
    {
        case DCMOTOR_DIR_FORWARD:
            HAL_GPIO_WritePin(MOTOR_AIN1_GPIO_Port, MOTOR_AIN1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(MOTOR_AIN2_GPIO_Port, MOTOR_AIN2_Pin, GPIO_PIN_RESET);
            break;
        case DCMOTOR_DIR_BACKWARD:
            HAL_GPIO_WritePin(MOTOR_AIN1_GPIO_Port, MOTOR_AIN1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(MOTOR_AIN2_GPIO_Port, MOTOR_AIN2_Pin, GPIO_PIN_SET);
            break;
        case DCMOTOR_DIR_STOP:
        default:
            HAL_GPIO_WritePin(MOTOR_AIN1_GPIO_Port, MOTOR_AIN1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(MOTOR_AIN2_GPIO_Port, MOTOR_AIN2_Pin, GPIO_PIN_RESET);
            break;
    }
}

static void DCMotor_SetRightDirection(DCMotor_Direction_t dir)
{
    switch (dir)
    {
        case DCMOTOR_DIR_FORWARD:
            HAL_GPIO_WritePin(MOTOR_BIN1_GPIO_Port, MOTOR_BIN1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(MOTOR_BIN2_GPIO_Port, MOTOR_BIN2_Pin, GPIO_PIN_RESET);
            break;
        case DCMOTOR_DIR_BACKWARD:
            HAL_GPIO_WritePin(MOTOR_BIN1_GPIO_Port, MOTOR_BIN1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(MOTOR_BIN2_GPIO_Port, MOTOR_BIN2_Pin, GPIO_PIN_SET);
            break;
        case DCMOTOR_DIR_STOP:
        default:
            HAL_GPIO_WritePin(MOTOR_BIN1_GPIO_Port, MOTOR_BIN1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(MOTOR_BIN2_GPIO_Port, MOTOR_BIN2_Pin, GPIO_PIN_RESET);
            break;
    }
}

static void DCMotor_SetLeftPwmAbs(uint16_t duty_abs)
{
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim4) + 1U;
    uint32_t ccr = (arr * duty_abs) / 100U;
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, ccr);
}

static void DCMotor_SetRightPwmAbs(uint16_t duty_abs)
{
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim4) + 1U;
    uint32_t ccr = (arr * duty_abs) / 100U;
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, ccr);
}

void DCMotor_Init(void)
{
    /* 启动 PWM */
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);

    /* 启动编码器 */
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
    HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);

    __HAL_TIM_SET_COUNTER(&htim2, 0);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    s_last_cnt_left = 0;
    s_last_cnt_right = 0;

    DCMotor_Enable(1);
    DCMotor_SetDuty(0, 0);
}

void DCMotor_Enable(uint8_t enable)
{
    HAL_GPIO_WritePin(MOTOR_STBY_GPIO_Port, MOTOR_STBY_Pin, enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void DCMotor_SetDuty(int16_t left_duty_percent, int16_t right_duty_percent)
{
    left_duty_percent = DCMotor_ClampDuty(left_duty_percent);
    right_duty_percent = DCMotor_ClampDuty(right_duty_percent);

    s_motor.left_duty_percent = left_duty_percent;
    s_motor.right_duty_percent = right_duty_percent;

    /* 左轮方向 */
    if (left_duty_percent > 0)
    {
        s_motor.left_dir = DCMOTOR_DIR_FORWARD;
        DCMotor_SetLeftDirection(DCMOTOR_DIR_FORWARD);
        DCMotor_SetLeftPwmAbs((uint16_t)left_duty_percent);
    }
    else if (left_duty_percent < 0)
    {
        s_motor.left_dir = DCMOTOR_DIR_BACKWARD;
        DCMotor_SetLeftDirection(DCMOTOR_DIR_BACKWARD);
        DCMotor_SetLeftPwmAbs((uint16_t)(-left_duty_percent));
    }
    else
    {
        s_motor.left_dir = DCMOTOR_DIR_STOP;
        DCMotor_SetLeftDirection(DCMOTOR_DIR_STOP);
        DCMotor_SetLeftPwmAbs(0);
    }

    /* 右轮方向 */
    if (right_duty_percent > 0)
    {
        s_motor.right_dir = DCMOTOR_DIR_FORWARD;
        DCMotor_SetRightDirection(DCMOTOR_DIR_FORWARD);
        DCMotor_SetRightPwmAbs((uint16_t)right_duty_percent);
    }
    else if (right_duty_percent < 0)
    {
        s_motor.right_dir = DCMOTOR_DIR_BACKWARD;
        DCMotor_SetRightDirection(DCMOTOR_DIR_BACKWARD);
        DCMotor_SetRightPwmAbs((uint16_t)(-right_duty_percent));
    }
    else
    {
        s_motor.right_dir = DCMOTOR_DIR_STOP;
        DCMotor_SetRightDirection(DCMOTOR_DIR_STOP);
        DCMotor_SetRightPwmAbs(0);
    }
}

void DCMotor_UpdateSpeed(float sample_time_s)
{
    if (sample_time_s <= 0.0f) return;

    int16_t now_left = (int16_t)__HAL_TIM_GET_COUNTER(&htim2);
    int16_t now_right = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);

    int16_t diff_left_16 = (int16_t)(now_left - s_last_cnt_left);
    int16_t diff_right_16 = (int16_t)(now_right - s_last_cnt_right);

    s_last_cnt_left = now_left;
    s_last_cnt_right = now_right;

    int32_t diff_left = (int32_t)diff_left_16;
    int32_t diff_right = (int32_t)diff_right_16;

    if (DCMOTOR_LEFT_ENCODER_REVERSE)
    {
        diff_left = -diff_left;
    }
    if (DCMOTOR_RIGHT_ENCODER_REVERSE)
    {
        diff_right = -diff_right;
    }

    s_motor.left_delta_count = diff_left;
    s_motor.right_delta_count = diff_right;

    const float wheel_circumference = DCMOTOR_WHEEL_DIAMETER_M * 3.1415926f;
    const float left_rev = ((float)diff_left) / DCMOTOR_ENCODER_CPR;
    const float right_rev = ((float)diff_right) / DCMOTOR_ENCODER_CPR;

    s_motor.left_speed_mps = (left_rev * wheel_circumference) / sample_time_s;
    s_motor.right_speed_mps = (right_rev * wheel_circumference) / sample_time_s;
}

void DCMotor_GetStatus(DCMotor_Status_t *status)
{
    if (status == 0) return;
    *status = s_motor;
}

const char *DCMotor_DirectionString(DCMotor_Direction_t dir)
{
    switch (dir)
    {
        case DCMOTOR_DIR_FORWARD:  return "FWD";
        case DCMOTOR_DIR_BACKWARD: return "BWD";
        case DCMOTOR_DIR_STOP:
        default:                   return "STP";
    }
}
