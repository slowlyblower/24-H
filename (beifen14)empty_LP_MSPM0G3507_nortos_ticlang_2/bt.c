/*
 * bt.c - 蓝牙串口 (硬件UART3: PA26=TX, PA25=RX, 9600bps)
 */
#include "bt.h"
#include "ti_msp_dl_config.h"

void bt_init(void)
{
    /* PA26 → UART3_TX (IOMUX_PINCM59, PF=2) */
    DL_GPIO_initPeripheralOutputFunction(IOMUX_PINCM59,
        IOMUX_PINCM59_PF_UART3_TX);
    /* PA25 → UART3_RX (IOMUX_PINCM55, PF=2) */
    DL_GPIO_initPeripheralInputFunction(IOMUX_PINCM55,
        IOMUX_PINCM55_PF_UART3_RX);

    /* 复位并上电 */
    DL_UART_Main_reset(UART3);
    DL_UART_Main_enablePower(UART3);

    /* 时钟: BUSCLK 32MHz */
    DL_UART_Main_ClockConfig clkCfg = {
        .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
        .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
    };
    DL_UART_Main_setClockConfig(UART3, &clkCfg);

    /* UART 配置: 8N1 */
    DL_UART_Main_Config uartCfg = {
        .mode        = DL_UART_MAIN_MODE_NORMAL,
        .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
        .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
        .parity      = DL_UART_MAIN_PARITY_NONE,
        .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
        .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
    };
    DL_UART_Main_init(UART3, &uartCfg);

    /* 波特率: 9600 (32MHz / 16 / 9600 = 208.33) */
    DL_UART_Main_setOversampling(UART3, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(UART3, 208, 21);

    DL_UART_Main_enable(UART3);
}

void bt_send_byte(char c)
{
    while (UART3->STAT & 0x80);   /* 等 TX FIFO 不满 (bit7 TXFF=0) */
    UART3->TXDATA = (uint32_t)c;
}

void bt_send_str(const char *s)
{
    while (*s) bt_send_byte(*s++);
}

void bt_send_num(int32_t n)
{
    char b[12]; int i = 0;
    if (n < 0) { bt_send_byte('-'); n = -n; }
    if (n == 0) { bt_send_byte('0'); return; }
    while (n) { b[i++] = '0' + (n % 10); n /= 10; }
    while (i)  bt_send_byte(b[--i]);
}
