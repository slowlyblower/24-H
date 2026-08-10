/*
 * gyro.h - BMI160 陀螺仪 (官方API + 软件I2C PB4/PB5)
 */
#ifndef GYRO_H
#define GYRO_H
#include <stdint.h>

void gyro_init(void);
void gyro_calibrate(uint16_t samples);
void gyro_update_bias(void);
void gyro_update(float dt);
float gyro_get_yaw(void);
float gyro_get_z(void);
int     gyro_get_err(void);
int     gyro_get_found(void);
int16_t gyro_get_raw(void);
void    gyro_set_yaw(float yaw);
void gyro_set_target_yaw(float yaw);
float gyro_get_yaw_error(void);
void gyro_get_rx(uint8_t *buf, uint8_t *cnt, uint8_t *done);

#endif
