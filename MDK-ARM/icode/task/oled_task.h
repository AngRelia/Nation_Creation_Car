#ifndef OLED_TASK_H
#define OLED_TASK_H

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "dc_motor.h"
#include "step_motor.h"

/* 应用 UI 运行模式定义。 */
typedef enum
{
	APP_MODE_IDLE = 0,
	APP_MODE_MOTOR,
	APP_MODE_STEPPER,
	APP_MODE_NRF,
	APP_MODE_ALL_CONTROL
} AppMode_t;

/* 主菜单光标位置定义。 */
typedef enum
{
	MENU_ITEM_MOTOR = 0,
	MENU_ITEM_STEPPER,
	MENU_ITEM_NRF,
	MENU_ITEM_ALL_CONTROL,
	MENU_ITEM_COUNT
} MenuItem_t;

#define APP_MODE_IS(mode) ((g_in_test == 1U) && (g_app_mode == (mode)))
#define DC_SPEED_MAX_MPS (2.0f)

/*
 * OLED 显示快照结构。
 * 生产者任务将该结构写入 OLED 队列，
 * OLED 任务按该结构渲染界面。
 */
typedef struct
{
	AppMode_t app_mode;
	uint8_t in_test;
	uint8_t menu_cursor;
	DCMotor_Status_t motor_status;
	uint8_t stepper_enable;
	StepperDir_t stepper_dir_l;
	StepperDir_t stepper_dir_r;
	float stepper_rpm_l;
	float stepper_rpm_r;
	uint8_t nrf_last_rx_flag;
	uint32_t nrf_rx_ok_count;
	uint32_t nrf_rx_err_count;
	uint8_t nrf_last_payload[4];
	uint8_t js_left;
	uint8_t js_right;
	float steer_target_deg;
	float steer_current_deg;
} OLEDDisplayData_t;

/* 对外可见的任务句柄、队列、互斥锁以及 UI 共享状态。 */
extern TaskHandle_t OLEDTaskHandle;
extern QueueHandle_t OLEDDataQueueHandle;
extern SemaphoreHandle_t AppDataMutexHandle;
extern volatile AppMode_t g_app_mode;
extern volatile uint8_t g_menu_cursor;
extern volatile uint8_t g_in_test;

/* 将最新共享状态发布到 OLED 队列（单槽覆盖写入）。 */
void OLED_PublishDisplayData(void);

/* 查询最新 OLED 显示快照（非破坏性读取）。 */
bool OLED_GetValue(OLEDDisplayData_t *out_data, uint32_t timeout_ms);

/* 创建 OLED 任务、显示队列与共享互斥锁资源。 */
void OLED_Task_Init(void);

/* 任务入口函数（通常作为 xTaskCreate 的入口参数）。 */
void OLED_Task_Entry(void *argument);

#endif
