#ifndef __PID_H
#define __PID_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct
{
    float kp;
    float ki;
    float kd;

    float integral;
    float prev_error;
    float output;

    float out_min;
    float out_max;
    float integral_min;
    float integral_max;
} PID_Controller_t;

void PID_Init(PID_Controller_t *pid,
              float kp,
              float ki,
              float kd,
              float out_min,
              float out_max,
              float integral_min,
              float integral_max);

void PID_Reset(PID_Controller_t *pid);

float PID_Calculate(PID_Controller_t *pid,
                    float setpoint,
                    float measurement,
                    float dt_s);

#ifdef __cplusplus
}
#endif

#endif
