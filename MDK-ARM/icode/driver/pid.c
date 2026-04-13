#include "pid.h"

static float PID_Clamp(float x, float min_v, float max_v)
{
    if (x < min_v) return min_v;
    if (x > max_v) return max_v;
    return x;
}

void PID_Init(PID_Controller_t *pid,
              float kp,
              float ki,
              float kd,
              float out_min,
              float out_max,
              float integral_min,
              float integral_max)
{
    if (pid == 0) return;

    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;

    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->output = 0.0f;

    pid->out_min = out_min;
    pid->out_max = out_max;
    pid->integral_min = integral_min;
    pid->integral_max = integral_max;
}

void PID_Reset(PID_Controller_t *pid)
{
    if (pid == 0) return;
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->output = 0.0f;
}

float PID_Calculate(PID_Controller_t *pid,
                    float setpoint,
                    float measurement,
                    float dt_s)
{
    float error;
    float p;
    float i;
    float d;
    float derivative;

    if (pid == 0) return 0.0f;
    if (dt_s <= 0.0f) return pid->output;

    error = setpoint - measurement;

    p = pid->kp * error;

    pid->integral += error * dt_s;
    pid->integral = PID_Clamp(pid->integral, pid->integral_min, pid->integral_max);
    i = pid->ki * pid->integral;

    derivative = (error - pid->prev_error) / dt_s;
    d = pid->kd * derivative;

    pid->output = p + i + d;
    pid->output = PID_Clamp(pid->output, pid->out_min, pid->out_max);

    pid->prev_error = error;
    return pid->output;
}
