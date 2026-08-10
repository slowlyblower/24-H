/*
 * gyro.c - 单轴陀螺仪模块 (UART2: PB17=TX, PA22=RX, 115200bps)
 *
 * 协议(模块发送):
 *   角速度: 0x5A 0xAA AzL AzH Sum  → raw/32768*2000 °/s
 *   角度:   0x5A 0xBB YawL YawH Sum → raw/32768*180°
 *   Sum = 0x5A + cmd + dataL + dataH (低8位)
 *
 * 指令(MCU发送→模块):
 *   解锁: 0x55 0xAA 0x13 0x8E 0x5F
 *   归零: 0x55 0xAA 0x15 0x00 0x00
 *   保存: 0x55 0xAA 0x00 0x00 0x00
 */
#include "gyro.h"
#include "ti_msp_dl_config.h"

/* ---- 内部状态 ---- */
static volatile float g_yaw = 0;
static volatile float g_target = 0;
static volatile float g_gz_raw = 0;
static int g_gyro_ok = 0;

/* 5字节帧解析 */
static uint8_t  g_frame[5];
static uint8_t  g_fidx = 0;
static volatile uint8_t g_new_yaw = 0;
static volatile uint8_t g_new_gz  = 0;

void UART2_IRQHandler(void)
{
    if (DL_UART_Main_getPendingInterrupt(UART2) != DL_UART_MAIN_IIDX_RX)
        return;
    uint8_t d = DL_UART_Main_receiveData(UART2);

    if (g_fidx == 0) {
        if (d == 0x5A) g_frame[g_fidx++] = d;
        return;
    }
    g_frame[g_fidx++] = d;
    if (g_fidx < 5) return;
    g_fidx = 0;

    /* 校验 */
    uint8_t sum = g_frame[0] + g_frame[1] + g_frame[2] + g_frame[3];
    if (sum != g_frame[4]) return;

    int16_t raw = (int16_t)((g_frame[3] << 8) | g_frame[2]);

    if (g_frame[1] == 0xAA) {
        g_gz_raw  = (float)raw / 32768.0f * 2000.0f;
        g_new_gz  = 1;
    } else if (g_frame[1] == 0xBB) {
        g_yaw    = (float)raw / 32768.0f * 180.0f;
        g_new_yaw = 1;
    }
}

static void gyro_send_cmd(const uint8_t *cmd)
{
    for (int i = 0; i < 5; i++) {
        while (UART2->STAT & 0x80);  /* 等TX FIFO不满 */
        UART2->TXDATA = cmd[i];
    }
}

void gyro_init(void)
{
    /* PB17 → UART2_TX (IOMUX_PINCM43, PF=2) */
    DL_GPIO_initPeripheralOutputFunction(IOMUX_PINCM43,
        IOMUX_PINCM43_PF_UART2_TX);
    /* PA22 → UART2_RX (IOMUX_PINCM47, PF=2) */
    DL_GPIO_initPeripheralInputFunction(IOMUX_PINCM47,
        IOMUX_PINCM47_PF_UART2_RX);

    DL_UART_Main_reset(UART2);
    DL_UART_Main_enablePower(UART2);

    DL_UART_Main_ClockConfig clk = {
        .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
        .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
    };
    DL_UART_Main_setClockConfig(UART2, &clk);

    DL_UART_Main_Config cfg = {
        .mode        = DL_UART_MAIN_MODE_NORMAL,
        .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
        .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
        .parity      = DL_UART_MAIN_PARITY_NONE,
        .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
        .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
    };
    DL_UART_Main_init(UART2, &cfg);

    DL_UART_Main_setOversampling(UART2, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(UART2, 17, 23);  /* 115200 */

    DL_UART_Main_enableInterrupt(UART2, DL_UART_MAIN_INTERRUPT_RX);
    NVIC_ClearPendingIRQ(UART2_INT_IRQn);
    NVIC_EnableIRQ(UART2_INT_IRQn);
    DL_UART_Main_enable(UART2);

    /* 发送解锁→归零→保存 */
    static const uint8_t key[]      = {0x55,0xAA,0x13,0x8E,0x5F};
    static const uint8_t yaw_zero[] = {0x55,0xAA,0x15,0x00,0x00};
    static const uint8_t save[]     = {0x55,0xAA,0x00,0x00,0x00};
    delay_cycles(320000);
    gyro_send_cmd(key);
    delay_cycles(3200000);
    gyro_send_cmd(yaw_zero);
    delay_cycles(3200000);
    gyro_send_cmd(save);
    delay_cycles(3200000);

    g_gyro_ok = 1;
    g_yaw = 0;
    g_target = 0;
}

void gyro_calibrate(uint16_t samples) { (void)samples; }
void gyro_update_bias(void) {}

void gyro_update(float dt)
{
    (void)dt;
    /* ISR已自动更新 g_yaw 和 g_gz_raw */
}

int   gyro_get_err(void)   { return 0; }
int   gyro_get_found(void)  { return g_gyro_ok; }
int16_t gyro_get_raw(void)  { return (int16_t)g_gz_raw; }
float gyro_get_yaw(void)   { return g_yaw; }
float gyro_get_z(void)     { return g_gz_raw; }
void  gyro_set_yaw(float yaw) { g_yaw = yaw; g_target = yaw; }
void  gyro_set_target_yaw(float yaw) { g_target = yaw; }

float gyro_get_yaw_error(void)
{
    float err = g_target - g_yaw;
    while (err >  180.0f) err -= 360.0f;
    while (err < -180.0f) err += 360.0f;
    return err;
}

/* 诊断接口 (兼容 Task10) */
void gyro_get_rx(uint8_t *buf, uint8_t *cnt, uint8_t *done)
{
    *cnt  = g_fidx;
    *done = g_new_yaw || g_new_gz;
    buf[0] = g_frame[0];
    buf[1] = g_frame[1];
    buf[2] = g_frame[2];
    buf[3] = g_frame[3];
}
