#ifndef __SERIAL_TASK_H
#define __SERIAL_TASK_H

#include "main.h"

/**
 * @brief 串口编号枚举
 */
typedef enum {
    PORT_UART1 = 0,
    PORT_UART2,
    PORT_UART3,
    PORT_UART4,
    PORT_MAX
} SerialPort_t;

void Serial_Task_Init(void);
void Serial_Task_Entry(void *argument);

/**
 * @brief 通用多串口打印接口
 * @param port 指定输出的串口编号
 * @param format 格式化字符串
 */
void AppSerial_Printf(SerialPort_t port, const char *format, ...);

#endif