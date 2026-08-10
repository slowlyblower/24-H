/*
 * pid.c - 位置式 PID
 * output = Kp*e + Ki*∫e·dt + Kd*de/dt
 */
#include "pid.h"
#include <string.h>

PID_t g_pid_line  = {0};
PID_t g_pid_angle = {0};

void pid_init(PID_t *pid, float kp, float ki, float kd,
             float out_min, float out_max)
{
    memset(pid, 0, sizeof(PID_t));
    pid->kp = kp; pid->ki = ki; pid->kd = kd;
    pid->out_min = out_min; pid->out_max = out_max;
}

/* 位置式 PID, 返回绝对值 */
float pid_compute(PID_t *pid, float error, float dt)
{
    /* P */
    float p_out = pid->kp * error;

    /* I (限幅) */
    pid->integral += error * dt;
    if (pid->integral >  35.0f) pid->integral =  30.0f;
    if (pid->integral < -35.0f) pid->integral = -30.0f;
    float i_out = pid->ki * pid->integral;

    /* D (低通防炸, 每PID独立) */
    float raw_d = (error - pid->last_error) / dt;
    pid->d_filter = 0.3f * raw_d + 0.7f * pid->d_filter;
    float d_out = pid->kd * pid->d_filter;

    pid->last_error = error;

    float out = p_out + i_out + d_out;
    if (out >  pid->out_max) out =  pid->out_max;
    if (out < -pid->out_max) out = -pid->out_max;
    return out;   /* 绝对值 */
}

void pid_reset(PID_t *pid)
{
    pid->integral   = 0.0f;
    pid->last_error = 0.0f;
    pid->d_filter   = 0.0f;
}
