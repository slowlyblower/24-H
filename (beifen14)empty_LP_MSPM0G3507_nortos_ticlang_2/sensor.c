/*
 * sensor.c - 8路灰度传感器 (UART串口模块, 115200bps)
 *
 * 协议: 0xAA + 指令字节 + 数据
 *   0x80/0x82~0x86: 16字节灰度数据 (8通道×2字节, 高字节在前)
 *   0x81:           1字节二值化数据 (bit0=CH0, bit7=CH7)
 *
 * 接线: PA10(TX), PA11(RX) — UART0
 */
#include "sensor.h"
#include "ti_msp_dl_config.h"

#define CHANNEL_NUM     8
#define GRAY_BUF_LEN    (CHANNEL_NUM * 2)

static volatile uint16_t sensor_gray[8];
static volatile uint8_t  sensor_bin;
static volatile uint8_t  rx_state = 0;
static         uint8_t  cmd_byte  = 0;
static         uint8_t  rx_buf[GRAY_BUF_LEN];
static volatile uint8_t  rx_idx   = 0;

void sensor_init(void)
{
    DL_GPIO_initPeripheralOutputFunction(IOMUX_PINCM1,
        IOMUX_PINCM1_PF_UART0_TX);
    /* 禁用 SysConfig 对 PA0 的 LED 输出 (避免与 UART 冲突) */
    GPIOA->DOE31_0 &= ~(1u << 0);
    DL_GPIO_initPeripheralInputFunction(IOMUX_PINCM2,
        IOMUX_PINCM2_PF_UART0_RX);

    DL_UART_Main_reset(UART0);
    DL_UART_Main_enablePower(UART0);

    DL_UART_Main_ClockConfig clk = {
        .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
        .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
    };
    DL_UART_Main_setClockConfig(UART0, &clk);

    DL_UART_Main_Config cfg = {
        .mode        = DL_UART_MAIN_MODE_NORMAL,
        .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
        .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
        .parity      = DL_UART_MAIN_PARITY_NONE,
        .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
        .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
    };
    DL_UART_Main_init(UART0, &cfg);

    DL_UART_Main_setOversampling(UART0, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(UART0, 17, 23);

    DL_UART_Main_enableInterrupt(UART0, DL_UART_MAIN_INTERRUPT_RX);
    NVIC_ClearPendingIRQ(UART0_INT_IRQn);
    NVIC_EnableIRQ(UART0_INT_IRQn);

    DL_UART_Main_enable(UART0);
}

void UART0_IRQHandler(void)
{
    if (DL_UART_Main_getPendingInterrupt(UART0) != DL_UART_MAIN_IIDX_RX)
        return;

    uint8_t d = DL_UART_Main_receiveData(UART0);

    if (rx_state == 0) {
        if (d == 0xAA) rx_state = 1;
        return;
    }
    if (rx_state == 1) {
        cmd_byte = d;
        rx_idx = 0;
        if (d == 0x80 || d == 0x82 || d == 0x83 || d == 0x84 || d == 0x85 || d == 0x86)
            rx_state = 2;
        else if (d == 0x81)
            rx_state = 3;
        else
            rx_state = 0;
        return;
    }
    if (rx_state == 2) {
        rx_buf[rx_idx] = d;  /* 顺序存储 */
        rx_idx++;
        if (rx_idx >= GRAY_BUF_LEN) {
            /* rx_buf[0]=CH0_H, [1]=CH0_L, [2]=CH1_H, [3]=CH1_L, ... */
            for (int i = 0; i < 8; i++)
                sensor_gray[i] = ((uint16_t)rx_buf[i*2] << 8) | rx_buf[i*2+1];
            rx_state = 0;
        }
        return;
    }
    if (rx_state == 3) {
        sensor_bin = d;
        rx_state = 0;
    }
}

void sensor_read_all(uint8_t buf[8])
{
    for (int i = 0; i < 8; i++)
        buf[i] = (sensor_bin >> i) & 1;
}

int8_t sensor_get_position(void)
{
    uint16_t vmin = 4095, vmax = 0;
    for (int i = 0; i < 8; i++) {
        if (sensor_gray[i] < vmin) vmin = sensor_gray[i];
        if (sensor_gray[i] > vmax) vmax = sensor_gray[i];
    }
    if (vmax - vmin < 500) return -128;  /* 对比度不足=无线 */

    uint16_t norm[8];
    for (int i = 0; i < 8; i++)
        norm[i] = (uint16_t)((uint32_t)(sensor_gray[i] - vmin) * 1000 / (vmax - vmin));

    const int16_t w[8] = {-70, -57, -28, -14, 14, 28, 57, 70};
    int32_t sum_w = 0, sum_v = 0;
    for (int i = 0; i < 8; i++) {
        sum_w += (int32_t)norm[i] * w[i];
        sum_v += norm[i];
    }
    if (sum_v == 0) return -128;
    int16_t pos = (int16_t)(sum_w / sum_v);
    if (pos < -100) pos = -100;
    if (pos >  100) pos =  100;
    return (int8_t)pos;
}

bool sensor_is_line_detected(void)
{
    return (sensor_get_position() != -128);
}

bool sensor_is_line_detected2(void)
{
    uint16_t vmin = 4095, vmax = 0;
    for (int i = 0; i < 8; i++) {
        if (sensor_gray[i] < vmin) vmin = sensor_gray[i];
        if (sensor_gray[i] > vmax) vmax = sensor_gray[i];
    }
    if (vmax - vmin < 500) return false;  /* 对比度太低=无线 */
    uint8_t cnt = 0;
    for (int i = 0; i < 8; i++)
        if (sensor_gray[i] - vmin > (vmax - vmin) / 3) cnt++;
    return (cnt >= 2);
}

bool sensor_is_fully_off_line(void)
{
    return !sensor_is_line_detected();
}

void sensor_get_gray(uint16_t buf[8])
{
    for (int i = 0; i < 8; i++)
        buf[i] = sensor_gray[i];
}
