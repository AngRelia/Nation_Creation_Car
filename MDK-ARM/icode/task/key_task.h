#ifndef KEY_TASK_H
#define KEY_TASK_H

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

/* 按键任务采样得到的最新原始按键状态（0 表示无按键按下）。 */
extern volatile uint8_t g_key_state;

/*
 * 初始化按键模块资源并创建按键扫描任务。
 * 这是按键模块推荐的对外初始化接口。
 */
void Key_TaskInit(void);

/* 为历史调用代码保留的兼容别名接口。 */
void Key_Task_Init(void);

/*
 * 从按键内部事件队列读取一个按键事件。
 * 在超时时间内成功读取到事件时返回 true。
 */
bool Key_GetValue(uint8_t *out_key, uint32_t timeout_ms);

/* 任务入口函数（通常作为 xTaskCreate 的入口参数）。 */
void Key_Task_Entry(void *argument);

#endif
