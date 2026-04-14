#include "serial_task.h"
#include "app_rtos_config.h"
#include "Serial.h"
#include "queue.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* --- 配置参数 --- */
#define MAX_PRINT_LEN     128  // 多串口可能需要更长的缓冲区
#define PRINT_QUEUE_SIZE  15   // 增大队列深度以缓存多个串口的数据

/**
 * @brief 串口消息结构体
 */
typedef struct {
    SerialPort_t port;           // 目标串口
    char buffer[MAX_PRINT_LEN];  // 字符串内容
} SerialMsg_t;

static TaskHandle_t xSerialTaskHandle = NULL;
static QueueHandle_t xPrintQueue = NULL;

void Serial_Task_Init(void)
{
    // 1. 初始化所有硬件串口 (目前你只有Serial2_Init，后续在这里增加其他串口初始化)
    // Serial1_Init(); 
    Serial2_Init();
    // Serial3_Init();

    // 2. 创建统一的消息队列
    if (xPrintQueue == NULL)
    {
        xPrintQueue = xQueueCreate(PRINT_QUEUE_SIZE, sizeof(SerialMsg_t));
    }

    // 3. 创建串口管理任务
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

void AppSerial_Printf(SerialPort_t port, const char *format, ...)
{
    if (xPrintQueue == NULL || port >= PORT_MAX) return;

    SerialMsg_t msg;
    msg.port = port; // 标记目标串口

    va_list args;
    va_start(args, format);
    vsnprintf(msg.buffer, sizeof(msg.buffer), format, args);
    va_end(args);

    // 发送到队列，不等待
    xQueueSend(xPrintQueue, &msg, 0);
}

void Serial_Task_Entry(void *argument)
{
    SerialMsg_t rxMsg;
    (void)argument;

    while (1)
    {
        // 阻塞等待队列。不管是哪个串口的消息，只要有消息进来就唤醒
        if (xQueueReceive(xPrintQueue, &rxMsg, portMAX_DELAY) == pdTRUE)
        {
            // 根据消息中的 port 标签，分发给不同的底层驱动
            switch (rxMsg.port)
            {
                case PORT_UART1:
                    // Serial1_SendString(rxMsg.buffer);
                    break;

                case PORT_UART2:
                    // 调用你现有的 Serial2 底层 DMA 发送函数
                    Serial2_SendString(rxMsg.buffer);
                    break;

                case PORT_UART3:
                    // Serial3_SendString(rxMsg.buffer);
                    break;

                default:
                    break;
            }
        }
    }
}