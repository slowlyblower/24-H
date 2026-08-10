/*
 * bt.h - 蓝牙串口 (硬件UART2: PB17=TX, PA22=RX)
 */
#ifndef BT_H
#define BT_H

#include <stdint.h>

void bt_init(void);
void bt_send_byte(char c);
void bt_send_str(const char *s);
void bt_send_num(int32_t n);

#endif
