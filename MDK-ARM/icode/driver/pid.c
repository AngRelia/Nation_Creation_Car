#include "pid.h"

PID_TypeDef PID_Left_Speed;
PID_TypeDef PID_Right_Speed;

float PID_Calculate_Step(PID_TypeDef *pid, float target, float actual)
{
    if (pid == 0)
    {
        return 0.0f;
    }

    pid->Error1 = pid->Error0;
    pid->Error0 = target - actual;

    if (pid->Ki != 0.0f)
    {
        pid->ErrorInt += pid->Error0;
        if (pid->ErrorInt > pid->IntegralMax) pid->ErrorInt = pid->IntegralMax;
        if (pid->ErrorInt < pid->IntegralMin) pid->ErrorInt = pid->IntegralMin;
    }

    pid->Output = pid->Kp * pid->Error0 +
                  pid->Ki * pid->ErrorInt +
                  pid->Kd * (pid->Error0 - pid->Error1);

    if (pid->Output > pid->OutputMax) pid->Output = pid->OutputMax;
    if (pid->Output < pid->OutputMin) pid->Output = pid->OutputMin;

    return pid->Output;
}

void PID_Init(void)
{
    PID_Left_Speed.Kp = 70.0f;
    PID_Left_Speed.Ki = 1.3f;
    PID_Left_Speed.Kd = 0.0f;

    PID_Left_Speed.OutputMax = 100.0f;
    PID_Left_Speed.OutputMin = -100.0f;
    PID_Left_Speed.IntegralMax = 90.0f;
    PID_Left_Speed.IntegralMin = -90.0f;

    PID_Left_Speed.Error0 = 0.0f;
    PID_Left_Speed.Error1 = 0.0f;
    PID_Left_Speed.ErrorInt = 0.0f;
    PID_Left_Speed.Output = 0.0f;

    PID_Right_Speed.Kp = 70.0f;
    PID_Right_Speed.Ki = 1.3f;
    PID_Right_Speed.Kd = 0.0f;

    PID_Right_Speed.OutputMax = 100.0f;
    PID_Right_Speed.OutputMin = -100.0f;
    PID_Right_Speed.IntegralMax = 90.0f;
    PID_Right_Speed.IntegralMin = -90.0f;

    PID_Right_Speed.Error0 = 0.0f;
    PID_Right_Speed.Error1 = 0.0f;
    PID_Right_Speed.ErrorInt = 0.0f;
    PID_Right_Speed.Output = 0.0f;
}

void PID_SetParameters(PID_ITEM item, float kp, float ki, float kd)
{
    PID_TypeDef *pid = 0;
    switch (item)
    {
        case MOTOR_LEFT:
            pid = &PID_Left_Speed;
            break;
        case MOTOR_RIGHT:
            pid = &PID_Right_Speed;
            break;
        default:
            return;
    }

    if (pid == 0)
    {
        return;
    }

    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;
}

void PID_Reset(PID_ITEM item)
{
    PID_TypeDef *pid = 0;
    switch (item)
    {
        case MOTOR_LEFT:
            pid = &PID_Left_Speed;
            break;
        case MOTOR_RIGHT:
            pid = &PID_Right_Speed;
            break;
        default:
            return;
    }

    if (pid == 0)
    {
        return;
    }

    pid->Error0 = 0.0f;
    pid->Error1 = 0.0f;
    pid->ErrorInt = 0.0f;
    pid->Output = 0.0f;
}

float PID_GetOutput(PID_ITEM item)
{
    PID_TypeDef *pid = 0;
    switch (item)
    {
        case MOTOR_LEFT:
            pid = &PID_Left_Speed;
            break;
        case MOTOR_RIGHT:
            pid = &PID_Right_Speed;
            break;
        default:
            return 0.0f;
    }

    if (pid == 0)
    {
        return 0.0f;
    }
    return pid->Output;
}
