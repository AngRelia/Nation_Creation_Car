#ifndef MOTOR_TASK_H
#define MOTOR_TASK_H

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "dc_motor.h"
#include "pid.h"

/* 对外可见的任务句柄与共享控制状态。 */
extern TaskHandle_t MotorTaskHandle;
extern volatile DCMotor_Status_t g_MotorStatus;
extern volatile int16_t g_all_motor_duty;
extern volatile float g_dc_target_mps;

/* 提供给其他任务读取的电机状态快照结构。 */
typedef struct
{
	DCMotor_Status_t motor_status;
	int16_t all_motor_duty;
	float dc_target_mps;
} MotorTaskData_t;

/* 创建电机任务以及内部状态快照队列。 */
void Motor_Task_Init(void);

/* 查询最新电机快照（非破坏性读取，不弹出队列数据）。 */
bool Motor_GetValue(MotorTaskData_t *out_data, uint32_t timeout_ms);

/* 任务入口函数（通常作为 xTaskCreate 的入口参数）。 */
void Motor_Task_Entry(void *argument);

#endif
