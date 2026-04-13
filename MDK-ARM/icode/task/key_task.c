#include "key_task.h"

#include "queue.h"

#include "app_rtos_config.h"

#include "main.h"

/*
 * 按键任务模块说明
 * ---------------
 * - 以固定周期扫描 4 个按键输入。
 * - 在“松手沿”上报按键事件，避免长按期间重复触发。
 * - 通过内部队列对外提供按键事件读取接口（Key_GetValue）。
 */

/* 模块内部私有资源 */
static TaskHandle_t xKeyTaskHandle = NULL;
static QueueHandle_t xKeyQueue = NULL;

volatile uint8_t g_key_state = 0U;

/* 直接读取 GPIO 引脚，得到当前原始按键状态。 */
static uint8_t Key_GetState(void)
{
    if (HAL_GPIO_ReadPin(KEY_1_GPIO_Port, KEY_1_Pin) == GPIO_PIN_RESET) { return 1U; }
    if (HAL_GPIO_ReadPin(KEY_2_GPIO_Port, KEY_2_Pin) == GPIO_PIN_RESET) { return 2U; }
    if (HAL_GPIO_ReadPin(KEY_3_GPIO_Port, KEY_3_Pin) == GPIO_PIN_RESET) { return 3U; }
    if (HAL_GPIO_ReadPin(KEY_4_GPIO_Port, KEY_4_Pin) == GPIO_PIN_RESET) { return 4U; }
    return 0U;
}

/* 推荐初始化流程：先创建事件队列，再创建按键扫描任务。 */
void Key_TaskInit(void)
{
    if (xKeyQueue == NULL)
    {
        xKeyQueue = xQueueCreate(KEY_EVENT_QUEUE_LENGTH, sizeof(uint8_t));
    }

    if ((xKeyQueue != NULL) && (xKeyTaskHandle == NULL))
    {
        xTaskCreate(Key_Task_Entry,
                    "KeyTask",
                    KEY_TASK_STACK_SIZE,
                    NULL,
                    KEY_TASK_PRIORITY,
                    &xKeyTaskHandle);
    }
}

/* 历史命名兼容接口。 */
void Key_Task_Init(void)
{
    Key_TaskInit();
}

/* 对外队列接口：其他任务可在超时内读取一个按键事件。 */
bool Key_GetValue(uint8_t *out_key, uint32_t timeout_ms)
{
    if ((out_key == NULL) || (xKeyQueue == NULL))
    {
        return false;
    }

    if (xQueueReceive(xKeyQueue, out_key, pdMS_TO_TICKS(timeout_ms)) == pdPASS)
    {
        return true;
    }

    return false;
}

/*
 * 按键扫描任务：
 * - 使用 vTaskDelayUntil 保持 20ms 固定节拍；
 * - 松手沿判定条件：(prev != 0) 且 (curr == 0)。
 */
void Key_Task_Entry(void *argument)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(20);
    uint8_t CurrState = 0U;
    uint8_t PrevState = 0U;

    (void)argument;

    while (1)
    {
        uint8_t key_pressed;

        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        PrevState = CurrState;
        CurrState = Key_GetState();
        g_key_state = CurrState;

        /* 用户松开按键时，仅入队一次该按键值。 */
        if ((CurrState == 0U) && (PrevState != 0U))
        {
            key_pressed = PrevState;

            if (xKeyQueue != NULL)
            {
                (void)xQueueSend(xKeyQueue, &key_pressed, 0U);
            }
        }
    }
}
