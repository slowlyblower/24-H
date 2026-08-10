/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
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
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)


#define CPUCLK_FREQ                                                     32000000



/* Defines for MOTOR_L_PWM */
#define MOTOR_L_PWM_INST                                                   TIMG0
#define MOTOR_L_PWM_INST_IRQHandler                             TIMG0_IRQHandler
#define MOTOR_L_PWM_INST_INT_IRQN                               (TIMG0_INT_IRQn)
#define MOTOR_L_PWM_INST_CLK_FREQ                                       32000000
/* GPIO defines for channel 0 */
#define GPIO_MOTOR_L_PWM_C0_PORT                                           GPIOA
#define GPIO_MOTOR_L_PWM_C0_PIN                                   DL_GPIO_PIN_12
#define GPIO_MOTOR_L_PWM_C0_IOMUX                                (IOMUX_PINCM34)
#define GPIO_MOTOR_L_PWM_C0_IOMUX_FUNC               IOMUX_PINCM34_PF_TIMG0_CCP0
#define GPIO_MOTOR_L_PWM_C0_IDX                              DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_MOTOR_L_PWM_C1_PORT                                           GPIOA
#define GPIO_MOTOR_L_PWM_C1_PIN                                   DL_GPIO_PIN_13
#define GPIO_MOTOR_L_PWM_C1_IOMUX                                (IOMUX_PINCM35)
#define GPIO_MOTOR_L_PWM_C1_IOMUX_FUNC               IOMUX_PINCM35_PF_TIMG0_CCP1
#define GPIO_MOTOR_L_PWM_C1_IDX                              DL_TIMER_CC_1_INDEX



/* Defines for CTRL_TIMER */
#define CTRL_TIMER_INST                                                 (TIMG12)
#define CTRL_TIMER_INST_IRQHandler                             TIMG12_IRQHandler
#define CTRL_TIMER_INST_INT_IRQN                               (TIMG12_INT_IRQn)
#define CTRL_TIMER_INST_LOAD_VALUE                                        (327U)




/* Port definition for Pin Group GPIO_LED */
#define GPIO_LED_PORT                                                    (GPIOA)

/* Defines for PIN_0: GPIOA.0 with pinCMx 1 on package pin 33 */
#define GPIO_LED_PIN_0_PIN                                       (DL_GPIO_PIN_0)
#define GPIO_LED_PIN_0_IOMUX                                      (IOMUX_PINCM1)
/* Port definition for Pin Group MOTOR_L_DIR */
#define MOTOR_L_DIR_PORT                                                 (GPIOA)

/* Defines for PIN_1: GPIOA.14 with pinCMx 36 on package pin 7 */
#define MOTOR_L_DIR_PIN_1_PIN                                   (DL_GPIO_PIN_14)
#define MOTOR_L_DIR_PIN_1_IOMUX                                  (IOMUX_PINCM36)
/* Port definition for Pin Group MOTOR_R_DIR */
#define MOTOR_R_DIR_PORT                                                 (GPIOA)

/* Defines for PIN_2: GPIOA.7 with pinCMx 14 on package pin 49 */
#define MOTOR_R_DIR_PIN_2_PIN                                    (DL_GPIO_PIN_7)
#define MOTOR_R_DIR_PIN_2_IOMUX                                  (IOMUX_PINCM14)
/* Port definition for Pin Group MOTOR_L_IN2 */
#define MOTOR_L_IN2_PORT                                                 (GPIOA)

/* Defines for PIN_3: GPIOA.15 with pinCMx 37 on package pin 8 */
#define MOTOR_L_IN2_PIN_3_PIN                                   (DL_GPIO_PIN_15)
#define MOTOR_L_IN2_PIN_3_IOMUX                                  (IOMUX_PINCM37)
/* Port definition for Pin Group MOTOR_R_IN2 */
#define MOTOR_R_IN2_PORT                                                 (GPIOA)

/* Defines for PIN_4: GPIOA.16 with pinCMx 38 on package pin 9 */
#define MOTOR_R_IN2_PIN_4_PIN                                   (DL_GPIO_PIN_16)
#define MOTOR_R_IN2_PIN_4_IOMUX                                  (IOMUX_PINCM38)
/* Port definition for Pin Group BUZZER */
#define BUZZER_PORT                                                      (GPIOB)

/* Defines for PIN_6: GPIOB.13 with pinCMx 30 on package pin 1 */
#define BUZZER_PIN_6_PIN                                        (DL_GPIO_PIN_13)
#define BUZZER_PIN_6_IOMUX                                       (IOMUX_PINCM30)
/* Port definition for Pin Group RGB_LED_R */
#define RGB_LED_R_PORT                                                   (GPIOB)

/* Defines for PIN_7: GPIOB.22 with pinCMx 50 on package pin 21 */
#define RGB_LED_R_PIN_7_PIN                                     (DL_GPIO_PIN_22)
#define RGB_LED_R_PIN_7_IOMUX                                    (IOMUX_PINCM50)
/* Port definition for Pin Group ENC_L */
#define ENC_L_PORT                                                       (GPIOB)

/* Defines for PIN_5: GPIOB.18 with pinCMx 44 on package pin 15 */
// pins affected by this interrupt request:["PIN_5"]
#define ENC_L_INT_IRQN                                          (GPIOB_INT_IRQn)
#define ENC_L_INT_IIDX                          (DL_INTERRUPT_GROUP1_IIDX_GPIOB)
#define ENC_L_PIN_5_IIDX                                    (DL_GPIO_IIDX_DIO18)
#define ENC_L_PIN_5_PIN                                         (DL_GPIO_PIN_18)
#define ENC_L_PIN_5_IOMUX                                        (IOMUX_PINCM44)
/* Port definition for Pin Group ENC_R */
#define ENC_R_PORT                                                       (GPIOA)

/* Defines for PIN_8: GPIOA.24 with pinCMx 54 on package pin 25 */
// pins affected by this interrupt request:["PIN_8"]
#define ENC_R_INT_IRQN                                          (GPIOA_INT_IRQn)
#define ENC_R_INT_IIDX                          (DL_INTERRUPT_GROUP1_IIDX_GPIOA)
#define ENC_R_PIN_8_IIDX                                    (DL_GPIO_IIDX_DIO24)
#define ENC_R_PIN_8_PIN                                         (DL_GPIO_PIN_24)
#define ENC_R_PIN_8_IOMUX                                        (IOMUX_PINCM54)


/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_MOTOR_L_PWM_init(void);
void SYSCFG_DL_CTRL_TIMER_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
