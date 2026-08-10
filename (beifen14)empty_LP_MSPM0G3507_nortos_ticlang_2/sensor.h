/*
 * sensor.h - 8路灰度传感器 (UART串口模块, 115200bps)
 *
 * 协议: 0xAA + 指令字节 + 数据
 *   0x80/0x82~0x86: 16字节灰度数据 (8通道×2字节, 高字节在前)
 *   0x81:           1字节二值化数据
 *
 * 接线: PA10(TX)←接灰度模块RX, PA11(RX)←接灰度模块TX
 */

#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>
#include <stdbool.h>

/* ---- API ---- */

/* 初始化 UART0 + 中断 */
void sensor_init(void);

/* 读取8路二值化数据 (0=白, 1=黑), 存入 buf[8] */
void sensor_read_all(uint8_t buf[8]);

/* 用灰度值计算连续位置: -100(极左) ~ 0(居中) ~ +100(极右), -128=无线 */
int8_t sensor_get_position(void);

/* 是否检测到黑线 (至少1路) */
bool sensor_is_line_detected(void);

/* 入弯检测 (至少2路) */
bool sensor_is_line_detected2(void);

/* 是否完全离线 (8路全白) */
bool sensor_is_fully_off_line(void);

/* 获取8路灰度值 (0~4095), 存入 buf[8] */
void sensor_get_gray(uint16_t buf[8]);

#endif
