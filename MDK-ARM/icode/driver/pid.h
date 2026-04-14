#ifndef __PID_H
#define __PID_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
    MOTOR_LEFT = 0,
    MOTOR_RIGHT
} PID_ITEM;

typedef struct
{
    float Kp;
    float Ki;
    float Kd;

    float Error0;
    float Error1;
    float ErrorInt;

    float Output;
    float OutputMax;
    float OutputMin;
    float IntegralMax;
    float IntegralMin;
} PID_TypeDef;

extern PID_TypeDef PID_Left_Speed;
extern PID_TypeDef PID_Right_Speed;

void PID_Init(void);
float PID_Calculate_Step(PID_TypeDef *pid, float target, float actual);
void PID_SetParameters(PID_ITEM item, float kp, float ki, float kd);
void PID_Reset(PID_ITEM item);
float PID_GetOutput(PID_ITEM item);

#ifdef __cplusplus
}
#endif

#endif
