/*
 * encoder.h - 编码器速度+里程 (SysConfig GPIO中断)
 * 车轮直径65mm, 轮距120mm, 编码器334线/转
 */
#ifndef ENCODER_H
#define ENCODER_H
#include <stdint.h>
#include <stdbool.h>

#define WHEEL_DIAMETER_MM   65.0f
#define WHEEL_CIRCUM_CM     (3.1415926f * 6.5f)
#define ENCODER_PPR         334
#define PULSE_PER_CM        (ENCODER_PPR / WHEEL_CIRCUM_CM)

void encoder_init(void);
void encoder_update(void);
void encoder_get_speed(float *l, float *r);
float encoder_get_left_distance(void);
float encoder_get_right_distance(void);
float encoder_get_distance(void);
void encoder_reset_distance(void);

#endif
