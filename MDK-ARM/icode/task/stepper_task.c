#include "stepper_task.h"

#include <math.h>

#include "queue.h"

#include "app_rtos_config.h"
#include "oled_task.h"
#include "nrf_task.h"

/*
 * 步进任务模块说明
 * ----------------
 * - 负责步进测试模式与总控模式下的转向控制。
 * - 维护转向目标角与当前估计角。
 * - 通过队列发布最新步进状态快照。
 */

TaskHandle_t StepperTaskHandle = NULL;
static QueueHandle_t xStepperDataQueue = NULL;
volatile float g_stepper_rpm_l = 0.0f;
volatile float g_stepper_rpm_r = 0.0f;
volatile StepperDir_t g_stepper_dir_l = STEPPER_DIR_CW;
volatile StepperDir_t g_stepper_dir_r = STEPPER_DIR_CW;
volatile uint8_t g_stepper_enable = 0U;
volatile float g_steer_target_deg = 0.0f;
volatile float g_steer_current_deg = 0.0f;

/* 将最新步进/转向状态发布到单槽队列。 */
static void Stepper_PublishData(void)
{
    StepperTaskData_t data;

    if (xStepperDataQueue == NULL)
    {
        return;
    }

    data.stepper_enable = g_stepper_enable;
    data.stepper_dir_l = g_stepper_dir_l;
    data.stepper_dir_r = g_stepper_dir_r;
    data.stepper_rpm_l = g_stepper_rpm_l;
    data.stepper_rpm_r = g_stepper_rpm_r;
    data.steer_target_deg = g_steer_target_deg;
    data.steer_current_deg = g_steer_current_deg;
    (void)xQueueOverwrite(xStepperDataQueue, &data);
}

/* 创建步进快照队列并启动步进任务。 */
void Stepper_Task_Init(void)
{
    if (xStepperDataQueue == NULL)
    {
        xStepperDataQueue = xQueueCreate(STEPPER_DATA_QUEUE_LENGTH, sizeof(StepperTaskData_t));
    }

    xTaskCreate(Stepper_Task_Entry,
                "StepperTask",
                STEPPER_TASK_STACK_SIZE,
                NULL,
                STEPPER_TASK_PRIORITY,
                &StepperTaskHandle);
}

/* 读取最新步进快照，不从队列中移除该数据。 */
bool Stepper_GetValue(StepperTaskData_t *out_data, uint32_t timeout_ms)
{
    if ((out_data == NULL) || (xStepperDataQueue == NULL))
    {
        return false;
    }

    if (xQueuePeek(xStepperDataQueue, out_data, pdMS_TO_TICKS(timeout_ms)) == pdPASS)
    {
        return true;
    }

    return false;
}

/*
 * 步进控制主循环：
 * - 以 20ms 固定步长更新，保证转向控制节拍稳定。
 * - APP_MODE_STEPPER：执行分阶段方向/转速测试序列。
 * - APP_MODE_ALL_CONTROL：按摇杆输入进行目标角跟踪。
 */
void Stepper_Task_Entry(void *argument)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(20);

    uint32_t stage = 0U;
    uint32_t stage_ms = 0U;
    uint8_t was_active = 0U;
    AppMode_t last_mode = APP_MODE_IDLE;

    (void)argument;

    Stepper_Init();
    Stepper_SetEnable(STEPPER_LEFT, 0);
    Stepper_SetEnable(STEPPER_RIGHT, 0);
    g_stepper_enable = 0U;

    g_stepper_dir_l = STEPPER_DIR_CW;
    g_stepper_dir_r = STEPPER_DIR_CW;
    Stepper_SetDir(STEPPER_LEFT, g_stepper_dir_l);
    Stepper_SetDir(STEPPER_RIGHT, g_stepper_dir_r);

    g_stepper_rpm_l = 60.0f;
    g_stepper_rpm_r = 90.0f;
    Stepper_SetSpeedRpm(STEPPER_LEFT, g_stepper_rpm_l);
    Stepper_SetSpeedRpm(STEPPER_RIGHT, g_stepper_rpm_r);

    while (1)
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        if (!(APP_MODE_IS(APP_MODE_STEPPER) || APP_MODE_IS(APP_MODE_ALL_CONTROL)))
        {
            if (was_active != 0U)
            {
                Stepper_SetSpeedRpm(STEPPER_LEFT, 0.0f);
                Stepper_SetSpeedRpm(STEPPER_RIGHT, 0.0f);
                Stepper_SetEnable(STEPPER_LEFT, 0);
                Stepper_SetEnable(STEPPER_RIGHT, 0);
                g_stepper_enable = 0U;
                was_active = 0U;
            }

            stage = 0U;
            stage_ms = 0U;
            g_steer_target_deg = 0.0f;
            g_steer_current_deg = 0.0f;
            last_mode = APP_MODE_IDLE;
            continue;
        }

        if (was_active == 0U)
        {
            Stepper_SetEnable(STEPPER_LEFT, 1);
            Stepper_SetEnable(STEPPER_RIGHT, 1);
            g_stepper_enable = 1U;
            was_active = 1U;
        }

        if (APP_MODE_IS(APP_MODE_STEPPER))
        {
            if (last_mode != APP_MODE_STEPPER)
            {
                stage = 0U;
                stage_ms = 0U;
            }

            stage_ms += 20U;
            if (stage_ms >= 3000U)
            {
                stage_ms = 0U;
                stage = (stage + 1U) % 4U;

                /* 轮换预设测试工况。 */
                if (stage == 0U)
                {
                    g_stepper_dir_l = STEPPER_DIR_CW;
                    g_stepper_dir_r = STEPPER_DIR_CW;
                    g_stepper_rpm_l = 60.0f;
                    g_stepper_rpm_r = 90.0f;
                }
                else if (stage == 1U)
                {
                    g_stepper_dir_l = STEPPER_DIR_CCW;
                    g_stepper_dir_r = STEPPER_DIR_CCW;
                    g_stepper_rpm_l = 120.0f;
                    g_stepper_rpm_r = 120.0f;
                }
                else if (stage == 2U)
                {
                    g_stepper_dir_l = STEPPER_DIR_CW;
                    g_stepper_dir_r = STEPPER_DIR_CCW;
                    g_stepper_rpm_l = 30.0f;
                    g_stepper_rpm_r = 60.0f;
                }
                else
                {
                    g_stepper_dir_l = STEPPER_DIR_CCW;
                    g_stepper_dir_r = STEPPER_DIR_CW;
                    g_stepper_rpm_l = 0.0f;
                    g_stepper_rpm_r = 0.0f;
                }

                Stepper_SetDir(STEPPER_LEFT, g_stepper_dir_l);
                Stepper_SetDir(STEPPER_RIGHT, g_stepper_dir_r);
                Stepper_SetSpeedRpm(STEPPER_LEFT, g_stepper_rpm_l);
                Stepper_SetSpeedRpm(STEPPER_RIGHT, g_stepper_rpm_r);
            }
        }
        else if (APP_MODE_IS(APP_MODE_ALL_CONTROL))
        {
            const float max_steer_deg = 35.0f;
            const float max_rpm = 160.0f;
            const float min_rpm = 30.0f;
            const float stop_err_deg = 1.0f;
            float right_diff = (float)((int16_t)g_js_right - (int16_t)JS_RIGHT_CENTER);
            float target;
            float err;
            float abs_err;
            float rpm;
            float moved_deg;
            float ratio;

            if (last_mode != APP_MODE_ALL_CONTROL)
            {
                g_steer_current_deg = 0.0f;
            }

            if (right_diff > 5.0f)
            {
                target = (right_diff / (255.0f - (float)JS_RIGHT_CENTER)) * max_steer_deg;
            }
            else if (right_diff < -5.0f)
            {
                target = (right_diff / (float)JS_RIGHT_CENTER) * max_steer_deg;
            }
            else
            {
                target = 0.0f;
            }

            g_steer_target_deg = target;

            err = g_steer_target_deg - g_steer_current_deg;
            abs_err = fabsf(err);
            if (abs_err <= stop_err_deg)
            {
                /* 偏差足够小时保持当前位置，停止转动。 */
                g_stepper_rpm_l = 0.0f;
                g_stepper_rpm_r = 0.0f;
                Stepper_SetSpeedRpm(STEPPER_LEFT, 0.0f);
                Stepper_SetSpeedRpm(STEPPER_RIGHT, 0.0f);
                g_steer_current_deg = g_steer_target_deg;
            }
            else
            {
                ratio = abs_err / max_steer_deg;
                if (ratio > 1.0f)
                {
                    ratio = 1.0f;
                }
                rpm = min_rpm + (max_rpm - min_rpm) * ratio;

                g_stepper_dir_l = (err > 0.0f) ? STEPPER_DIR_CW : STEPPER_DIR_CCW;
                g_stepper_dir_r = g_stepper_dir_l;
                g_stepper_rpm_l = rpm;
                g_stepper_rpm_r = rpm;

                Stepper_SetDir(STEPPER_LEFT, g_stepper_dir_l);
                Stepper_SetDir(STEPPER_RIGHT, g_stepper_dir_r);
                Stepper_SetSpeedRpm(STEPPER_LEFT, g_stepper_rpm_l);
                Stepper_SetSpeedRpm(STEPPER_RIGHT, g_stepper_rpm_r);

                /* 按控制周期积分估算转向角位移。 */
                moved_deg = rpm * 360.0f / 60.0f * 0.02f;
                if (err > 0.0f)
                {
                    g_steer_current_deg += moved_deg;
                    if (g_steer_current_deg > g_steer_target_deg)
                    {
                        g_steer_current_deg = g_steer_target_deg;
                    }
                }
                else
                {
                    g_steer_current_deg -= moved_deg;
                    if (g_steer_current_deg < g_steer_target_deg)
                    {
                        g_steer_current_deg = g_steer_target_deg;
                    }
                }
            }
        }

        last_mode = g_app_mode;

        /* 向外部读取方和 OLED 任务发布最新快照。 */
        Stepper_PublishData();
        OLED_PublishDisplayData();
    }
}
