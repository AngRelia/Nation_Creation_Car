#ifndef SERIAL_TASK_H
#define SERIAL_TASK_H

#include "FreeRTOS.h"
#include "task.h"

void Serial_Task_Init(void);
void Serial_Task_Entry(void *argument);

#endif
