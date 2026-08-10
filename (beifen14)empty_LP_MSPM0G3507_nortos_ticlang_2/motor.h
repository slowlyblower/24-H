/*
 * motor.h - TB6612FNG 双电机驱动 (SysConfig 引脚)
 *
 * 左: PA8(PWM) PA10(IN1) PA11(IN2)
 * 右: PB0(PWM) PB1(IN1) PB7(IN2)
 * STBY: PB14
 */

#ifndef MOTOR_H
#define MOTOR_H
#include <stdint.h>

void motor_init(void);
void motor_left_set(int16_t pct);
void motor_right_set(int16_t pct);
void motor_set(int16_t left, int16_t right);
void motor_brake(void);
void motor_coast(void);

#endif
