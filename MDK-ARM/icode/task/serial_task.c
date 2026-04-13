#include "serial_task.h"

#include "app_rtos_config.h"
#include "Serial.h"

static TaskHandle_t xSerialTaskHandle = NULL;

void Serial_Task_Init(void)
{
    Serial2_Init();

    if (xSerialTaskHandle == NULL)
    {
        xTaskCreate(Serial_Task_Entry,
                    "SerialTask",
                    SERIAL_TASK_STACK_SIZE,
                    NULL,
                    SERIAL_TASK_PRIORITY,
                    &xSerialTaskHandle);
    }
}

void Serial_Task_Entry(void *argument)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(1000U);

    (void)argument;

    while (1)
    {
        Serial2_SendString("HB:++\r\n");
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
