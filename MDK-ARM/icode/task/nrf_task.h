#ifndef NRF_TASK_H
#define NRF_TASK_H

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "nrf24l01.h"

#define JS_LEFT_CENTER (130U)
#define JS_RIGHT_CENTER (123U)
#define JS_DEADZONE (8)
#define NRF_LOST_TIMEOUT_MS (200U)

/* 对外可见的任务句柄与最新无线/摇杆状态。 */
extern TaskHandle_t NrfTaskHandle;
extern volatile uint8_t g_nrf_last_rx_flag;
extern volatile uint32_t g_nrf_rx_ok_count;
extern volatile uint32_t g_nrf_rx_err_count;
extern volatile uint8_t g_nrf_last_payload[NRF24L01_RX_PACKET_WIDTH];
extern volatile TickType_t g_nrf_last_ok_tick;
extern volatile uint8_t g_js_left;
extern volatile uint8_t g_js_right;

/* 供跨任务读取的 NRF 状态快照。 */
typedef struct
{
	uint8_t nrf_last_rx_flag;
	uint32_t nrf_rx_ok_count;
	uint32_t nrf_rx_err_count;
	uint8_t nrf_last_payload[NRF24L01_RX_PACKET_WIDTH];
	TickType_t nrf_last_ok_tick;
	uint8_t js_left;
	uint8_t js_right;
} NrfTaskData_t;

/* 创建 NRF 任务及内部状态快照队列资源。 */
void Nrf_Task_Init(void);

/* 查询最新 NRF 快照（非破坏性读取）。 */
bool Nrf_GetValue(NrfTaskData_t *out_data, uint32_t timeout_ms);

/* 任务入口函数（通常作为 xTaskCreate 的入口参数）。 */
void Nrf_Task_Entry(void *argument);

#endif
