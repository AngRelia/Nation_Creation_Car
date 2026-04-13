#include "step_motor.h"
#include "tim.h"

static void Stepper_WriteEn(GPIO_TypeDef *port, uint16_t pin, uint8_t enable)
{
#if STEPPER_EN_ACTIVE_LOW
    HAL_GPIO_WritePin(port, pin, enable ? GPIO_PIN_RESET : GPIO_PIN_SET);
#else
    HAL_GPIO_WritePin(port, pin, enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
#endif
}

void Stepper_Init(void)
{
    /* 关闭使能，默认停转 */
    Stepper_SetEnable(STEPPER_LEFT, 0);
    Stepper_SetEnable(STEPPER_RIGHT, 0);

    /* 默认方向 */
    Stepper_SetDir(STEPPER_LEFT, STEPPER_DIR_CW);
    Stepper_SetDir(STEPPER_RIGHT, STEPPER_DIR_CW);

    /* 启动 PWM 输出（STEP） */
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_2);
}

void Stepper_SetEnable(StepperId_t id, uint8_t enable)
{
    if (id == STEPPER_LEFT)
    {
        Stepper_WriteEn(STEPPER_L_EN_GPIO_Port, STEPPER_L_EN_Pin, enable);
    }
    else
    {
        Stepper_WriteEn(STEPPER_R_EN_GPIO_Port, STEPPER_R_EN_Pin, enable);
    }
}

void Stepper_SetDir(StepperId_t id, StepperDir_t dir)
{
    GPIO_PinState level = (dir == STEPPER_DIR_CW) ? GPIO_PIN_RESET : GPIO_PIN_SET;

    if (id == STEPPER_LEFT)
    {
        HAL_GPIO_WritePin(STEPPER_L_DIR_GPIO_Port, STEPPER_L_DIR_Pin, level);
    }
    else
    {
        HAL_GPIO_WritePin(STEPPER_R_DIR_GPIO_Port, STEPPER_R_DIR_Pin, level);
    }
}

static void Stepper_SetTimerFreq(TIM_HandleTypeDef *htim, uint32_t channel, uint32_t step_hz)
{
    if (step_hz == 0)
    {
        __HAL_TIM_SET_COMPARE(htim, channel, 0);
        return;
    }

    /* 计数器时钟 = TIM9CLK / (PSC+1) */
    uint32_t tim_clk;
    if (htim->Instance == TIM9)
    {
        tim_clk = HAL_RCC_GetPCLK2Freq();
        if ((RCC->CFGR & RCC_CFGR_PPRE2) != RCC_CFGR_PPRE2_DIV1)
        {
            tim_clk *= 2U;
        }
    }
    else
    {
        tim_clk = HAL_RCC_GetPCLK1Freq();
        if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1)
        {
            tim_clk *= 2U;
        }
    }

    uint32_t psc = htim->Init.Prescaler;
    uint32_t cnt_clk = tim_clk / (psc + 1U);
    uint32_t arr = (cnt_clk / step_hz) - 1U;

    __HAL_TIM_DISABLE(htim);
    __HAL_TIM_SET_AUTORELOAD(htim, arr);
    __HAL_TIM_SET_COMPARE(htim, channel, (arr + 1U) / 2U);
    __HAL_TIM_ENABLE(htim);
}

void Stepper_SetStepFreq(StepperId_t id, uint32_t step_hz)
{
    if (id == STEPPER_LEFT)
    {
        Stepper_SetTimerFreq(&htim5, TIM_CHANNEL_3, step_hz);
    }
    else
    {
        Stepper_SetTimerFreq(&htim9, TIM_CHANNEL_2, step_hz);
    }
}

void Stepper_SetSpeedRpm(StepperId_t id, float rpm)
{
    if (rpm <= 0.0f)
    {
        Stepper_SetStepFreq(id, 0);
        return;
    }

    float step_hz_f = (rpm * (float)STEPPER_PULSES_PER_REV) / 60.0f;
    uint32_t step_hz = (uint32_t)(step_hz_f + 0.5f);
    Stepper_SetStepFreq(id, step_hz);
}
