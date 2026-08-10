/*
 * vision.h - 视觉模块 (UART1: PA8=TX, PA9=RX)
 */
#ifndef VISION_H
#define VISION_H

#include <stdint.h>

/* 钢球坐标 */
extern volatile uint16_t g_ball_x;
extern volatile uint16_t g_ball_y;
extern volatile uint8_t  g_ball_valid;

void vision_init(void);

#endif
