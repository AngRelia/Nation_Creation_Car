#ifndef STEPPER_TASK_H
#define STEPPER_TASK_H

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "step_motor.h"

/* 对外可见的任务句柄与共享转向状态。 */
extern TaskHandle_t StepperTaskHandle;
extern volatile float g_stepper_rpm_l;
extern volatile float g_stepper_rpm_r;
extern volatile StepperDir_t g_stepper_dir_l;
extern volatile StepperDir_t g_stepper_dir_r;
extern volatile uint8_t g_stepper_enable;
extern volatile float g_steer_target_deg;
extern volatile float g_steer_current_deg;

/* 供跨任务读取的步进/转向状态快照。 */
typedef struct
{
	uint8_t stepper_enable;
	StepperDir_t stepper_dir_l;
	StepperDir_t stepper_dir_r;
	float stepper_rpm_l;
	float stepper_rpm_r;
	float steer_target_deg;
	float steer_current_deg;
} StepperTaskData_t;

/* 创建步进任务及内部状态快照队列资源。 */
void Stepper_Task_Init(void);

/* 查询最新步进/转向快照（非破坏性读取）。 */
bool Stepper_GetValue(StepperTaskData_t *out_data, uint32_t timeout_ms);

/* 任务入口函数（通常作为 xTaskCreate 的入口参数）。 */
void Stepper_Task_Entry(void *argument);

#endif
