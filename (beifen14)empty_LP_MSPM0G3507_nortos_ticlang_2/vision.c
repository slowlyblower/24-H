/*
 * vision.c - 视觉模块 (UART1: PA8=TX, PA9=RX, 115200bps)
 *
 * 坐标帧: AA AA XH XL YH YL Check FF FF
 */
#include "vision.h"
#include "ti_msp_dl_config.h"

static volatile uint8_t rx_frame[9];
static volatile uint8_t rx_index = 0;

volatile uint16_t g_ball_x = 0;
volatile uint16_t g_ball_y = 0;
volatile uint8_t  g_ball_valid = 0;

static void restart_from_byte(uint8_t data)
{
    if (data == 0xAA) { rx_frame[0] = data; rx_index = 1; }
    else rx_index = 0;
}

static void publish_coordinate(void)
{
    uint8_t cs = (uint8_t)(rx_frame[2]+rx_frame[3]+rx_frame[4]+rx_frame[5]);
    if (cs != rx_frame[6]) return;
    g_ball_x = ((uint16_t)rx_frame[2]<<8) | rx_frame[3];
    g_ball_y = ((uint16_t)rx_frame[4]<<8) | rx_frame[5];
    g_ball_valid = 1;
}

void vision_parse_byte(uint8_t data)
{
    switch (rx_index) {
        case 0: restart_from_byte(data); break;
        case 1:
            if (data == 0xAA) { rx_frame[1]=data; rx_index=2; }
            else restart_from_byte(data);
            break;
        case 2: case 3: case 4: case 5: case 6:
            rx_frame[rx_index]=data; rx_index++; break;
        case 7:
            if (data == 0xFF) { rx_frame[7]=data; rx_index=8; }
            else restart_from_byte(data);
            break;
        case 8:
            if (data == 0xFF) { rx_frame[8]=data; publish_coordinate(); }
            rx_index = 0;
            break;
        default: rx_index=0; break;
    }
}

void UART1_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART1)) {
        case DL_UART_MAIN_IIDX_RX:
            vision_parse_byte(DL_UART_Main_receiveData(UART1));
            break;
        default: break;
    }
}

void vision_init(void)
{
    /* PA8 → UART1_TX (IOMUX_PINCM19, PF=2) */
    DL_GPIO_initPeripheralOutputFunction(IOMUX_PINCM19,
        IOMUX_PINCM19_PF_UART1_TX);
    /* PA9 → UART1_RX (IOMUX_PINCM20, PF=2) */
    DL_GPIO_initPeripheralInputFunction(IOMUX_PINCM20,
        IOMUX_PINCM20_PF_UART1_RX);

    DL_UART_Main_reset(UART1);
    DL_UART_Main_enablePower(UART1);

    DL_UART_Main_ClockConfig clkCfg = {
        .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
        .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
    };
    DL_UART_Main_setClockConfig(UART1, &clkCfg);

    DL_UART_Main_Config uartCfg = {
        .mode        = DL_UART_MAIN_MODE_NORMAL,
        .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
        .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
        .parity      = DL_UART_MAIN_PARITY_NONE,
        .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
        .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
    };
    DL_UART_Main_init(UART1, &uartCfg);

    /* 115200bps: 32MHz/16/115200=17.36 */
    DL_UART_Main_setOversampling(UART1, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(UART1, 17, 23);

    DL_UART_Main_enableInterrupt(UART1, DL_UART_MAIN_INTERRUPT_RX);
    NVIC_ClearPendingIRQ(UART1_INT_IRQn);
    NVIC_EnableIRQ(UART1_INT_IRQn);

    DL_UART_Main_enable(UART1);
}
