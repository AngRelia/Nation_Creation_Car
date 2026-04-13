#ifndef APP_RTOS_CONFIG_H
#define APP_RTOS_CONFIG_H

#include "FreeRTOS.h"
#include "task.h"

/* 任务栈大小（单位为 word，不是 byte）。 */
#define MOTOR_TASK_STACK_SIZE    256U
#define OLED_TASK_STACK_SIZE     512U
#define KEY_TASK_STACK_SIZE      256U
#define STEPPER_TASK_STACK_SIZE  256U
#define NRF_TASK_STACK_SIZE      256U

/* 任务优先级（相对空闲任务 tskIDLE_PRIORITY）。 */
#define MOTOR_TASK_PRIORITY      (tskIDLE_PRIORITY + 3U)
#define OLED_TASK_PRIORITY       (tskIDLE_PRIORITY + 1U)
#define KEY_TASK_PRIORITY        (tskIDLE_PRIORITY + 2U)
#define STEPPER_TASK_PRIORITY    (tskIDLE_PRIORITY + 3U)
#define NRF_TASK_PRIORITY        (tskIDLE_PRIORITY + 3U)

/* 事件通道和快照通道的队列长度。 */
#define KEY_EVENT_QUEUE_LENGTH   8U
#define OLED_DATA_QUEUE_LENGTH   1U
#define MOTOR_DATA_QUEUE_LENGTH  1U
#define STEPPER_DATA_QUEUE_LENGTH 1U
#define NRF_DATA_QUEUE_LENGTH    1U

/* 访问 UI/应用共享状态时，互斥锁等待超时。 */
#define APP_MUTEX_TIMEOUT_TICKS  pdMS_TO_TICKS(2U)

#endif
