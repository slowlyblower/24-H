/*
 * pid.h - 增量式 PID 控制器 (双实例)
 *
 * 实例1: 循迹 PID  输入=黑线位置(-100~+100), 输出=转向差速
 * 实例2: 角度保持 PID 输入=偏航角偏差(°), 输出=转向差速
 */

#ifndef PID_H
#define PID_H

#include <stdint.h>
#include <stdbool.h>

/* PID 参数结构体 */
typedef struct {
    float kp, ki, kd;
    float integral;
    float last_error;
    float d_filter;        /* D项低通滤波值 (每个PID独立) */
    float out_min, out_max;
} PID_t;

/* ---- API ---- */

/* 初始化 PID 实例 */
void pid_init(PID_t *pid, float kp, float ki, float kd,
             float out_min, float out_max);

/* 增量式 PID 计算: 返回增量值 */
/* error = target - actual, dt = 控制周期(秒) */
float pid_compute(PID_t *pid, float error, float dt);

/* 重置 PID 状态 (清积分 + 清误差缓存) */
void pid_reset(PID_t *pid);

/* 预定义的 PID 实例 (extern) */
extern PID_t g_pid_line;   /* 循迹 PID */
extern PID_t g_pid_angle;  /* 角度保持 PID */

#endif
