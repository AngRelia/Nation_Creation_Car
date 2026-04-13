#include "motor_task.h"

#include <math.h>

#include "queue.h"

#include "app_rtos_config.h"
#include "nrf_task.h"
#include "oled_task.h"
#include "stepper_task.h"

#include "Serial.h"

/*
 * 电机任务模块说明
 * ----------------
 * - 负责直流电机测试模式与总控模式下的执行逻辑。
 * - 在总控模式中维护速度闭环 PID 控制。
 * - 通过队列发布电机状态快照，供其他任务读取。
 */

TaskHandle_t MotorTaskHandle = NULL;
static QueueHandle_t xMotorDataQueue = NULL;
volatile DCMotor_Status_t g_MotorStatus;
volatile int16_t g_all_motor_duty = 0;
volatile float g_dc_target_mps = 0.0f;
PID_Controller_t g_dc_pid;

/* 将最新电机状态发布到单槽队列（覆盖旧值语义）。 */
static void Motor_PublishData(void)
{
    MotorTaskData_t data;

    if (xMotorDataQueue == NULL)
    {
        return;
    }

    data.motor_status = g_MotorStatus;
    data.all_motor_duty = g_all_motor_duty;
    data.dc_target_mps = g_dc_target_mps;
    (void)xQueueOverwrite(xMotorDataQueue, &data);
}

/* 创建电机快照队列并启动电机任务。 */
void Motor_Task_Init(void)
{
    if (xMotorDataQueue == NULL)
    {
        xMotorDataQueue = xQueueCreate(MOTOR_DATA_QUEUE_LENGTH, sizeof(MotorTaskData_t));
    }

    xTaskCreate(Motor_Task_Entry,
                "MotorTask",
                MOTOR_TASK_STACK_SIZE,
                NULL,
                MOTOR_TASK_PRIORITY,
                &MotorTaskHandle);
}

/* 读取最新电机快照，不从队列中移除该数据。 */
bool Motor_GetValue(MotorTaskData_t *out_data, uint32_t timeout_ms)
{
    if ((out_data == NULL) || (xMotorDataQueue == NULL))
    {
        return false;
    }

    if (xQueuePeek(xMotorDataQueue, out_data, pdMS_TO_TICKS(timeout_ms)) == pdPASS)
    {
        return true;
    }

    return false;
}

/*
 * 电机控制主循环：
 * - 以 50ms 固定步长运行，保证控制节拍稳定。
 * - APP_MODE_MOTOR：执行分阶段开环测试。
 * - APP_MODE_ALL_CONTROL：摇杆目标 + PID 闭环速度控制。
 */
void Motor_Task_Entry(void *argument)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(50);
    uint32_t stage = 0U;
    uint32_t stage_ms = 0U;

    (void)argument;
    DCMotor_Init();
    DCMotor_SetDuty(0, 0);

    PID_Init(&g_dc_pid,
             70.0f,
             65.0f,
             0.0f,
             -100.0f,
             100.0f,
             -1.8f,
             1.8f);

    while (1)
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        if (APP_MODE_IS(APP_MODE_MOTOR))
        {
            stage_ms += 50U;
            if (stage_ms >= 2000U)
            {
                stage_ms = 0U;
                stage = (stage + 1U) % 4U;
            }

            switch (stage)
            {
                case 0U: DCMotor_SetDuty(35, 35); break;
                case 1U: DCMotor_SetDuty(-35, -35); break;
                case 2U: DCMotor_SetDuty(25, 45); break;
                default: DCMotor_SetDuty(0, 0); break;
            }
        }
        else if (APP_MODE_IS(APP_MODE_ALL_CONTROL))
        {
            TickType_t now_tick = xTaskGetTickCount();
            int16_t diff;
            float target_mps = 0.0f;
            static float target_cmd_mps = 0.0f;
            float meas_mps;
            static float meas_mps_filt = 0.0f;
            float duty_f;
            float speed_err;
            int16_t duty;

            if ((now_tick - g_nrf_last_ok_tick) > pdMS_TO_TICKS(NRF_LOST_TIMEOUT_MS))
            {
                g_js_left = JS_LEFT_CENTER;
                g_js_right = JS_RIGHT_CENTER;
            }

            diff = (int16_t)g_js_left - (int16_t)JS_LEFT_CENTER;
            if (diff > JS_DEADZONE)
            {
                target_mps = ((float)diff / (255.0f - (float)JS_LEFT_CENTER)) * DC_SPEED_MAX_MPS;
            }
            else if (diff < -JS_DEADZONE)
            {
                target_mps = ((float)diff / (float)JS_LEFT_CENTER) * DC_SPEED_MAX_MPS;
            }

            /* 对目标速度施加斜率限制，避免指令突变。 */
            {
                const float max_step = 0.10f;
                float delta = target_mps - target_cmd_mps;
                if (delta > max_step)
                {
                    delta = max_step;
                }
                if (delta < -max_step)
                {
                    delta = -max_step;
                }
                target_cmd_mps += delta;
                if (fabsf(target_mps) < 0.03f && fabsf(target_cmd_mps) < 0.06f)
                {
                    target_cmd_mps = 0.0f;
                }
            }

            g_dc_target_mps = target_cmd_mps;
            meas_mps = (g_MotorStatus.left_speed_mps + g_MotorStatus.right_speed_mps) * 0.5f;
            meas_mps_filt = meas_mps_filt * 0.7f + meas_mps * 0.3f;
            speed_err = g_dc_target_mps - meas_mps_filt;

            if (fabsf(g_dc_target_mps) < 0.03f)
            {
                PID_Reset(&g_dc_pid);
                duty = 0;
            }
            else
            {
                duty_f = PID_Calculate(&g_dc_pid, g_dc_target_mps, meas_mps_filt, 0.05f);
                duty = (int16_t)duty_f;
            }

            /* 将 PWM 占空比限制在有效范围内。 */
            if (duty > 100)
            {
                duty = 100;
            }
            if (duty < -100)
            {
                duty = -100;
            }

            g_all_motor_duty = duty;
            DCMotor_SetDuty(duty, duty);

            Serial2_Printf("%.3f,%.3f,%.3f,%d,%.3f,%.3f\n",
                           g_dc_target_mps,
                           meas_mps_filt,
                           speed_err,
                           duty,
                           g_steer_target_deg,
                           g_steer_current_deg);

            stage = 0U;
            stage_ms = 0U;
        }
        else
        {
            DCMotor_SetDuty(0, 0);
            g_all_motor_duty = 0;
            g_dc_target_mps = 0.0f;
            PID_Reset(&g_dc_pid);
            stage = 0U;
            stage_ms = 0U;
        }

        /* 刷新电机测量值，并向外发布最新快照。 */
        DCMotor_UpdateSpeed(0.05f);
        DCMotor_GetStatus((DCMotor_Status_t *)&g_MotorStatus);
        Motor_PublishData();
        OLED_PublishDisplayData();
    }
}
