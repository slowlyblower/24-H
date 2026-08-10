/*
 * Copyright (c) 2023, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.c =============
 *  Configured MSPM0 DriverLib module definitions
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */

#include "ti_msp_dl_config.h"

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform any initialization needed before using any board APIs
 */
SYSCONFIG_WEAK void SYSCFG_DL_init(void)
{
    SYSCFG_DL_initPower();
    SYSCFG_DL_GPIO_init();
    /* Module-Specific Initializations*/
    SYSCFG_DL_SYSCTL_init();
    SYSCFG_DL_MOTOR_L_PWM_init();
    SYSCFG_DL_CTRL_TIMER_init();
}



SYSCONFIG_WEAK void SYSCFG_DL_initPower(void)
{
    DL_GPIO_reset(GPIOA);
    DL_GPIO_reset(GPIOB);
    DL_TimerG_reset(MOTOR_L_PWM_INST);
    DL_TimerG_reset(CTRL_TIMER_INST);

    DL_GPIO_enablePower(GPIOA);
    DL_GPIO_enablePower(GPIOB);
    DL_TimerG_enablePower(MOTOR_L_PWM_INST);
    DL_TimerG_enablePower(CTRL_TIMER_INST);
    delay_cycles(POWER_STARTUP_DELAY);
}

SYSCONFIG_WEAK void SYSCFG_DL_GPIO_init(void)
{

    DL_GPIO_initPeripheralOutputFunction(GPIO_MOTOR_L_PWM_C0_IOMUX,GPIO_MOTOR_L_PWM_C0_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_MOTOR_L_PWM_C0_PORT, GPIO_MOTOR_L_PWM_C0_PIN);
    DL_GPIO_initPeripheralOutputFunction(GPIO_MOTOR_L_PWM_C1_IOMUX,GPIO_MOTOR_L_PWM_C1_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_MOTOR_L_PWM_C1_PORT, GPIO_MOTOR_L_PWM_C1_PIN);

    DL_GPIO_initDigitalOutput(GPIO_LED_PIN_0_IOMUX);

    DL_GPIO_initDigitalOutput(MOTOR_L_DIR_PIN_1_IOMUX);

    DL_GPIO_initDigitalOutput(MOTOR_R_DIR_PIN_2_IOMUX);

    DL_GPIO_initDigitalOutput(MOTOR_L_IN2_PIN_3_IOMUX);

    DL_GPIO_initDigitalOutput(MOTOR_R_IN2_PIN_4_IOMUX);

    DL_GPIO_initDigitalOutput(BUZZER_PIN_6_IOMUX);

    DL_GPIO_initDigitalOutput(RGB_LED_R_PIN_7_IOMUX);

    DL_GPIO_initDigitalInputFeatures(ENC_L_PIN_5_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(ENC_R_PIN_8_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_clearPins(GPIOA, GPIO_LED_PIN_0_PIN |
		MOTOR_L_DIR_PIN_1_PIN |
		MOTOR_R_DIR_PIN_2_PIN |
		MOTOR_L_IN2_PIN_3_PIN |
		MOTOR_R_IN2_PIN_4_PIN);
    DL_GPIO_enableOutput(GPIOA, GPIO_LED_PIN_0_PIN |
		MOTOR_L_DIR_PIN_1_PIN |
		MOTOR_R_DIR_PIN_2_PIN |
		MOTOR_L_IN2_PIN_3_PIN |
		MOTOR_R_IN2_PIN_4_PIN);
    DL_GPIO_setUpperPinsPolarity(GPIOA, DL_GPIO_PIN_24_EDGE_FALL);
    DL_GPIO_clearInterruptStatus(GPIOA, ENC_R_PIN_8_PIN);
    DL_GPIO_enableInterrupt(GPIOA, ENC_R_PIN_8_PIN);
    DL_GPIO_clearPins(GPIOB, BUZZER_PIN_6_PIN |
		RGB_LED_R_PIN_7_PIN);
    DL_GPIO_enableOutput(GPIOB, BUZZER_PIN_6_PIN |
		RGB_LED_R_PIN_7_PIN);
    DL_GPIO_setUpperPinsPolarity(GPIOB, DL_GPIO_PIN_18_EDGE_FALL);
    DL_GPIO_clearInterruptStatus(GPIOB, ENC_L_PIN_5_PIN);
    DL_GPIO_enableInterrupt(GPIOB, ENC_L_PIN_5_PIN);

}


SYSCONFIG_WEAK void SYSCFG_DL_SYSCTL_init(void)
{

	//Low Power Mode is configured to be SLEEP0
    DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);

    DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);
    /* Set default configuration */
    DL_SYSCTL_disableHFXT();
    DL_SYSCTL_disableSYSPLL();
    DL_SYSCTL_setULPCLKDivider(DL_SYSCTL_ULPCLK_DIV_1);
    DL_SYSCTL_setMCLKDivider(DL_SYSCTL_MCLK_DIVIDER_DISABLE);

}


/*
 * Timer clock configuration to be sourced by  / 1 (32000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   32000000 Hz = 32000000 Hz / (1 * (0 + 1))
 */
static const DL_TimerG_ClockConfig gMOTOR_L_PWMClockConfig = {
    .clockSel = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale = 0U
};

static const DL_TimerG_PWMConfig gMOTOR_L_PWMConfig = {
    .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN,
    .period = 1600,
    .isTimerWithFourCC = true,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_MOTOR_L_PWM_init(void) {

    DL_TimerG_setClockConfig(
        MOTOR_L_PWM_INST, (DL_TimerG_ClockConfig *) &gMOTOR_L_PWMClockConfig);

    DL_TimerG_initPWMMode(
        MOTOR_L_PWM_INST, (DL_TimerG_PWMConfig *) &gMOTOR_L_PWMConfig);

    // Set Counter control to the smallest CC index being used
    DL_TimerG_setCounterControl(MOTOR_L_PWM_INST,DL_TIMER_CZC_CCCTL0_ZCOND,DL_TIMER_CAC_CCCTL0_ACOND,DL_TIMER_CLC_CCCTL0_LCOND);

    DL_TimerG_setCaptureCompareOutCtl(MOTOR_L_PWM_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_DISABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERG_CAPTURE_COMPARE_0_INDEX);

    DL_TimerG_setCaptCompUpdateMethod(MOTOR_L_PWM_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERG_CAPTURE_COMPARE_0_INDEX);
    DL_TimerG_setCaptureCompareValue(MOTOR_L_PWM_INST, 1600, DL_TIMER_CC_0_INDEX);

    DL_TimerG_setCaptureCompareOutCtl(MOTOR_L_PWM_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_DISABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERG_CAPTURE_COMPARE_1_INDEX);

    DL_TimerG_setCaptCompUpdateMethod(MOTOR_L_PWM_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERG_CAPTURE_COMPARE_1_INDEX);
    DL_TimerG_setCaptureCompareValue(MOTOR_L_PWM_INST, 1600, DL_TIMER_CC_1_INDEX);

    DL_TimerG_enableClock(MOTOR_L_PWM_INST);


    
    DL_TimerG_setCCPDirection(MOTOR_L_PWM_INST , DL_TIMER_CC0_OUTPUT | DL_TIMER_CC1_OUTPUT );


}



/*
 * Timer clock configuration to be sourced by LFCLK /  (32768 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   32768 Hz = 32768 Hz / (1 * (0 + 1))
 */
static const DL_TimerG_ClockConfig gCTRL_TIMERClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_LFCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale    = 0U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * CTRL_TIMER_INST_LOAD_VALUE = (10 ms * 32768 Hz) - 1
 */
static const DL_TimerG_TimerConfig gCTRL_TIMERTimerConfig = {
    .period     = CTRL_TIMER_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_CTRL_TIMER_init(void) {

    DL_TimerG_setClockConfig(CTRL_TIMER_INST,
        (DL_TimerG_ClockConfig *) &gCTRL_TIMERClockConfig);

    DL_TimerG_initTimerMode(CTRL_TIMER_INST,
        (DL_TimerG_TimerConfig *) &gCTRL_TIMERTimerConfig);
    DL_TimerG_enableInterrupt(CTRL_TIMER_INST , DL_TIMERG_INTERRUPT_ZERO_EVENT);
    DL_TimerG_enableClock(CTRL_TIMER_INST);





}


