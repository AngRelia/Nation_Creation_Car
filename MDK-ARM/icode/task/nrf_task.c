#include "nrf_task.h"

#include "queue.h"

#include "app_rtos_config.h"
#include "oled_task.h"

/*
 * NRF 任务模块说明
 * ---------------
 * - 周期轮询 NRF24L01 接收器。
 * - 更新摇杆通道值与接收统计信息。
 * - 通过队列发布最新无线数据快照。
 */

TaskHandle_t NrfTaskHandle = NULL;
static QueueHandle_t xNrfDataQueue = NULL;
volatile uint8_t g_nrf_last_rx_flag = 0U;
volatile uint32_t g_nrf_rx_ok_count = 0U;
volatile uint32_t g_nrf_rx_err_count = 0U;
volatile uint8_t g_nrf_last_payload[NRF24L01_RX_PACKET_WIDTH] = {0U};
volatile TickType_t g_nrf_last_ok_tick = 0U;
volatile uint8_t g_js_left = JS_LEFT_CENTER;
volatile uint8_t g_js_right = JS_RIGHT_CENTER;

/* 将最新 NRF 状态发布到单槽队列。 */
static void Nrf_PublishData(void)
{
    NrfTaskData_t data;

    if (xNrfDataQueue == NULL)
    {
        return;
    }

    data.nrf_last_rx_flag = g_nrf_last_rx_flag;
    data.nrf_rx_ok_count = g_nrf_rx_ok_count;
    data.nrf_rx_err_count = g_nrf_rx_err_count;
    data.nrf_last_payload[0] = g_nrf_last_payload[0];
    data.nrf_last_payload[1] = g_nrf_last_payload[1];
    data.nrf_last_payload[2] = g_nrf_last_payload[2];
    data.nrf_last_payload[3] = g_nrf_last_payload[3];
    data.nrf_last_ok_tick = g_nrf_last_ok_tick;
    data.js_left = g_js_left;
    data.js_right = g_js_right;
    (void)xQueueOverwrite(xNrfDataQueue, &data);
}

/* 创建 NRF 快照队列并启动 NRF 任务。 */
void Nrf_Task_Init(void)
{
    if (xNrfDataQueue == NULL)
    {
        xNrfDataQueue = xQueueCreate(NRF_DATA_QUEUE_LENGTH, sizeof(NrfTaskData_t));
    }

    xTaskCreate(Nrf_Task_Entry,
                "NrfTask",
                NRF_TASK_STACK_SIZE,
                NULL,
                NRF_TASK_PRIORITY,
                &NrfTaskHandle);
}

/* 读取最新 NRF 快照，不从队列中移除该数据。 */
bool Nrf_GetValue(NrfTaskData_t *out_data, uint32_t timeout_ms)
{
    if ((out_data == NULL) || (xNrfDataQueue == NULL))
    {
        return false;
    }

    if (xQueuePeek(xNrfDataQueue, out_data, pdMS_TO_TICKS(timeout_ms)) == pdPASS)
    {
        return true;
    }

    return false;
}

/*
 * NRF 接收主循环：
 * - 10ms 周期轮询。
 * - 收到有效包后，对摇杆值执行低通滤波。
 */
void Nrf_Task_Entry(void *argument)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10);
    uint8_t rxFlag;

    (void)argument;

    NRF24L01_Init();

    while (1)
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        if (!(APP_MODE_IS(APP_MODE_NRF) || APP_MODE_IS(APP_MODE_ALL_CONTROL)))
        {
            continue;
        }

        rxFlag = NRF24L01_Receive();
        g_nrf_last_rx_flag = rxFlag;

        if (rxFlag == 1U)
        {
            uint16_t left_f;
            uint16_t right_f;

            g_nrf_rx_ok_count++;
            g_nrf_last_payload[0] = NRF24L01_RxPacket[0];
            g_nrf_last_payload[1] = NRF24L01_RxPacket[1];
            g_nrf_last_payload[2] = NRF24L01_RxPacket[2];
            g_nrf_last_payload[3] = NRF24L01_RxPacket[3];
            g_nrf_last_ok_tick = xTaskGetTickCount();

            /* 一阶滤波抑制摇杆 ADC 抖动。 */
            left_f = (uint16_t)g_js_left * 3U + (uint16_t)NRF24L01_RxPacket[0];
            right_f = (uint16_t)g_js_right * 3U + (uint16_t)NRF24L01_RxPacket[1];
            g_js_left = (uint8_t)(left_f / 4U);
            g_js_right = (uint8_t)(right_f / 4U);

            /* 在中心点附近进行吸附，抑制小幅漂移。 */
            if (((int16_t)g_js_left - (int16_t)JS_LEFT_CENTER) < 3 &&
                ((int16_t)g_js_left - (int16_t)JS_LEFT_CENTER) > -3)
            {
                g_js_left = JS_LEFT_CENTER;
            }
            if (((int16_t)g_js_right - (int16_t)JS_RIGHT_CENTER) < 3 &&
                ((int16_t)g_js_right - (int16_t)JS_RIGHT_CENTER) > -3)
            {
                g_js_right = JS_RIGHT_CENTER;
            }
        }
        else if (rxFlag >= 2U)
        {
            g_nrf_rx_err_count++;
        }

        /* 向外部读取方和 OLED 任务发布最新快照。 */
        Nrf_PublishData();
        OLED_PublishDisplayData();
    }
}
