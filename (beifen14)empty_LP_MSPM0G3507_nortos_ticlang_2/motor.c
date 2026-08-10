/*
 * motor.c - TB6612 双电机驱动 (SysConfig PWM + GPIO)
 * 正数=正转, 负数=反转, 0=惰行
 */
#include "motor.h"
#include "ti_msp_dl_config.h"

#define PWM_PERIOD  1600

void motor_init(void)
{
    DL_TimerG_enableClock(MOTOR_L_PWM_INST);
    DL_TimerG_startCounter(MOTOR_L_PWM_INST);
}

/* 左电机: >0正转, <0反转, 0惰行 */
void motor_left_set(int16_t pct)
{
    if (pct > 0) {
        DL_GPIO_setPins(MOTOR_L_DIR_PORT, MOTOR_L_DIR_PIN_1_PIN);
        DL_GPIO_clearPins(MOTOR_L_IN2_PORT, MOTOR_L_IN2_PIN_3_PIN);
    } else if (pct < 0) {
        pct = -pct;
        DL_GPIO_clearPins(MOTOR_L_DIR_PORT, MOTOR_L_DIR_PIN_1_PIN);
        DL_GPIO_setPins(MOTOR_L_IN2_PORT, MOTOR_L_IN2_PIN_3_PIN);
    } else {
        DL_GPIO_clearPins(MOTOR_L_DIR_PORT, MOTOR_L_DIR_PIN_1_PIN);
        DL_GPIO_clearPins(MOTOR_L_IN2_PORT, MOTOR_L_IN2_PIN_3_PIN);
    }
    if (pct > 100) pct = 100;
    uint16_t c = (uint16_t)((uint32_t)(100 - pct) * PWM_PERIOD / 100);
    DL_TimerG_setCaptureCompareValue(MOTOR_L_PWM_INST, c, DL_TIMER_CC_0_INDEX);
}

/* 右电机: >0正转, <0反转, 0惰行 */
void motor_right_set(int16_t pct)
{
    if (pct > 0) {
        DL_GPIO_clearPins(MOTOR_R_DIR_PORT, MOTOR_R_DIR_PIN_2_PIN);
        DL_GPIO_setPins(MOTOR_R_IN2_PORT, MOTOR_R_IN2_PIN_4_PIN);
    } else if (pct < 0) {
        pct = -pct;
        DL_GPIO_setPins(MOTOR_R_DIR_PORT, MOTOR_R_DIR_PIN_2_PIN);
        DL_GPIO_clearPins(MOTOR_R_IN2_PORT, MOTOR_R_IN2_PIN_4_PIN);
    } else {
        DL_GPIO_clearPins(MOTOR_R_DIR_PORT, MOTOR_R_DIR_PIN_2_PIN);
        DL_GPIO_clearPins(MOTOR_R_IN2_PORT, MOTOR_R_IN2_PIN_4_PIN);
    }
    if (pct > 100) pct = 100;
    uint16_t c = (uint16_t)((uint32_t)(100 - pct) * PWM_PERIOD / 100);
    DL_TimerG_setCaptureCompareValue(MOTOR_L_PWM_INST, c, DL_TIMER_CC_1_INDEX);
}

void motor_set(int16_t left, int16_t right)
{
    motor_left_set(right);
    motor_right_set(left);
}

void motor_brake(void)
{
    DL_GPIO_setPins(MOTOR_L_DIR_PORT, MOTOR_L_DIR_PIN_1_PIN);
    DL_GPIO_setPins(MOTOR_L_IN2_PORT, MOTOR_L_IN2_PIN_3_PIN);
    DL_GPIO_setPins(MOTOR_R_DIR_PORT, MOTOR_R_DIR_PIN_2_PIN);
    DL_GPIO_setPins(MOTOR_R_IN2_PORT, MOTOR_R_IN2_PIN_4_PIN);
}

void motor_coast(void)
{
    DL_GPIO_clearPins(MOTOR_L_DIR_PORT, MOTOR_L_DIR_PIN_1_PIN);
    DL_GPIO_clearPins(MOTOR_L_IN2_PORT, MOTOR_L_IN2_PIN_3_PIN);
    DL_GPIO_clearPins(MOTOR_R_DIR_PORT, MOTOR_R_DIR_PIN_2_PIN);
    DL_GPIO_clearPins(MOTOR_R_IN2_PORT, MOTOR_R_IN2_PIN_4_PIN);
}
