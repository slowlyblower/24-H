/*
 * encoder.c - SysConfig GPIO中断 + 速度PID内环
 */
#include "encoder.h"
#include "ti_msp_dl_config.h"

#define CONTROL_PERIOD_S  0.01f

volatile int32_t g_enc_l_cnt = 0;
volatile int32_t g_enc_r_cnt = 0;

static float g_speed_l = 0, g_speed_r = 0;
static float g_dist_l = 0, g_dist_r = 0;

void encoder_init(void)
{
    /* GPIO 层中断已由 SysConfig 配置: 下降沿触发 + IMASK 使能 */
    /* 这里补上 NVIC (CPU 中断控制器) 的使能 */
    NVIC_EnableIRQ(GPIOB_INT_IRQn);
    NVIC_EnableIRQ(GPIOA_INT_IRQn);
}

/* GROUP1 = GPIOB 中断: 左编码器 PB18 */
void GROUP1_IRQHandler(void)
{
    uint32_t st = DL_GPIO_getEnabledInterruptStatus(ENC_L_PORT, ENC_L_PIN_5_PIN);
    if (st & ENC_L_PIN_5_PIN) {
        g_enc_l_cnt++;
        DL_GPIO_clearInterruptStatus(ENC_L_PORT, ENC_L_PIN_5_PIN);
    }
}

/* GROUP0 = GPIOA 中断: 右编码器 PA24 */
void GROUP0_IRQHandler(void)
{
    uint32_t st = DL_GPIO_getEnabledInterruptStatus(ENC_R_PORT, ENC_R_PIN_8_PIN);
    if (st & ENC_R_PIN_8_PIN) {
        g_enc_r_cnt++;
        DL_GPIO_clearInterruptStatus(ENC_R_PORT, ENC_R_PIN_8_PIN);
    }
}

static int32_t g_last_l = 0, g_last_r = 0;

/* 每控制周期: 读脉冲增量 → 速度 */
void encoder_update(void)
{
    int32_t dl = g_enc_l_cnt - g_last_l;
    int32_t dr = g_enc_r_cnt - g_last_r;
    g_last_l = g_enc_l_cnt;
    g_last_r = g_enc_r_cnt;

    g_speed_l = (float)dl / PULSE_PER_CM / CONTROL_PERIOD_S;
    g_speed_r = (float)dr / PULSE_PER_CM / CONTROL_PERIOD_S;

    g_dist_l += (float)dl / PULSE_PER_CM;
    g_dist_r += (float)dr / PULSE_PER_CM;
}

void encoder_get_speed(float *l, float *r) { *l = g_speed_l; *r = g_speed_r; }
float encoder_get_left_distance(void)  { return g_dist_l; }
float encoder_get_right_distance(void) { return g_dist_r; }
float encoder_get_distance(void)       { return (g_dist_l + g_dist_r) / 2.0f; }

void encoder_reset_distance(void)
{
    g_dist_l = 0; g_dist_r = 0;
    g_last_l = g_enc_l_cnt; g_last_r = g_enc_r_cnt;
}
