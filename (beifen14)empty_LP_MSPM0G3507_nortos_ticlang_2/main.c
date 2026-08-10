/*
 * H题「自动行驶小车」- 任务1~4 循迹+盲走
 *
 * 控制逻辑:
 *   有线 → line_follow()  (加权位置 + PID, 参考8路灰度模块源码)
 *   无线 → blind_drive()  (陀螺仪锁角, P控制)
 *   顶点 → 刹车300ms → 声光提示 → 切下一段 → 重置PID/里程
 */

#include "ti_msp_dl_config.h"
#include "sensor.h"
#include "motor.h"
#include "encoder.h"
#include "gyro.h"
#include "pid.h"
#include "lcd.h"
#include "state_machine.h"
#include "bt.h"
#include "vision.h"

/* ================================================================
 *  速度参数 (直接 PWM 占空比 0~100)
 * ================================================================ */
#define SPEED_LINE    38      /* 循迹速度 */
#define SPEED_BLIND   46      /* 盲走速度 */
#define SPEED_T5      19      /* 任务5循迹速度 (砍半) */


/* ================================================================
 *  盲走陀螺仪 P 控制参数
 * ================================================================ */
#define KP_ANGLE      2.5f    /* 盲走锁角 P 增益 */
#define TURN_LIMIT    60.0f   /* 转向限幅 */

/* ================================================================
 *  全局状态
 * ================================================================ */
static uint8_t  g_cur_task  = 2;
static int16_t  g_last_L    = 0;       /* 上帧左轮 PWM */
static int16_t  g_last_R    = 0;       /* 上帧右轮 PWM */

/* ================================================================
 *  IO 引脚
 * ================================================================ */
#define LED_G_PORT   GPIOB
#define LED_G_PIN    DL_GPIO_PIN_24
#define LED_G_IOMUX  IOMUX_PINCM23
#define LED_B_PORT   GPIOB
#define LED_B_PIN    DL_GPIO_PIN_25
#define LED_B_IOMUX  IOMUX_PINCM27
#define KEY1_PORT    GPIOB
#define KEY1_PIN     DL_GPIO_PIN_21
#define KEY1_IOMUX   IOMUX_PINCM49
#define KEY2_PORT    GPIOB
#define KEY2_PIN     DL_GPIO_PIN_20
#define KEY2_IOMUX   IOMUX_PINCM48

/* ---- 基础函数 ---- */
void delay_ms(uint32_t ms) { delay_cycles(ms * 32000); }

static void beep(uint16_t ms)
{
    DL_GPIO_setPins(LED_B_PORT, LED_B_PIN);
    delay_ms(ms);
    DL_GPIO_clearPins(LED_B_PORT, LED_B_PIN);
}

static void extra_io_init(void)
{
    DL_GPIO_initDigitalOutput(LED_G_IOMUX);
    DL_GPIO_clearPins(LED_G_PORT, LED_G_PIN);
    DL_GPIO_enableOutput(LED_G_PORT, LED_G_PIN);
    DL_GPIO_initDigitalOutput(LED_B_IOMUX);
    DL_GPIO_clearPins(LED_B_PORT, LED_B_PIN);
    DL_GPIO_enableOutput(LED_B_PORT, LED_B_PIN);
    DL_GPIO_initDigitalInputFeatures(KEY1_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(KEY2_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
}

static uint8_t key1_read(void) { return DL_GPIO_readPins(KEY1_PORT, KEY1_PIN) ? 1 : 0; }
static uint8_t key2_read(void) { return DL_GPIO_readPins(KEY2_PORT, KEY2_PIN) ? 1 : 0; }

/* ---- 任务选择 ---- */
static uint8_t task_select(void)
{
    uint8_t task = 2;
    lcd_clear(LCD_BLACK);
    lcd_set_cursor(0, 0); lcd_puts("Select Task:");
    lcd_set_cursor(1, 0); lcd_puts("K1:+ K2:OK  T2");
    while (1) {
        if (key1_read() == 0) { delay_ms(50);
            if (key1_read() == 0) {
                task = (task % 10) + 1;  /* 1~10 循环 */
                lcd_set_cursor(1, 8); lcd_putc('0' + task); lcd_puts("  ");
                while (key1_read() == 0) delay_ms(10);
            }
        }
        if (key2_read() == 0) { delay_ms(50);
            if (key2_read() == 0) {
                while (key2_read() == 0) delay_ms(10);
                break;
            }
        }
        delay_ms(50);
    }
    lcd_clear(LCD_BLACK);
    return task;
}

/* ================================================================
 *  循迹控制: 加权位置 + PID (参考 8 路灰度模块源码)
 *
 *  丢线时 (pos == -128): 保持上帧误差, PID 状态连续, 不跳变
 *  有线时: 加权位置 → 死区 → 过零积分清零 → 动态积分限幅 → PID
 * ================================================================ */

void line_follow_reset(void)
{
    g_last_L = 0;
    g_last_R = 0;
}

/*
 * 灰度 + 位置低通滤波 + PD 循迹 (参考 MSPM0G3507_for_car)
 *
 * real = pos * 0.6 + last_pos * 0.4   // 低通滤波
 * out  = error * Kp + delta_error * Kd  // PD, 无积分
 */
#define RIF_TARGET  0.0f    /* 目标位置 (0=居中) */
#define RIF_KP      0.3f    /* P 增益 */
#define RIF_KD      1.2f    /* D 增益 (大KD防抖) */

void line_follow(void)
{
    int8_t pos = sensor_get_position();

    static float last_pos = 0, last_error = 0;
    float steer;

    if (pos == -128) {
        steer = 0;              /* 无线: 直走 */
        last_error = 0;         /* 重置误差历史 */
    } else {
        /* 低通滤波 */
        float filt = (float)pos * 0.6f + last_pos * 0.4f;
        last_pos = filt;

        /* PD */
        float error = RIF_TARGET - filt;
        steer = error * RIF_KP + (error - last_error) * RIF_KD;
        last_error = error;
    }

    if (steer >  40.0f) steer =  40.0f;
    if (steer < -40.0f) steer = -40.0f;

    int16_t L = (int16_t)((float)SPEED_LINE + steer);
    int16_t R = (int16_t)((float)SPEED_LINE - steer);
    L += 2;

    if (L < 0) L = 0; if (L > 100) L = 100;
    if (R < 0) R = 0; if (R > 100) R = 100;

    g_last_L = L;
    g_last_R = R;
    motor_set(L, R);
}

/* ================================================================
 *  陀螺仪控原地转向 (替代死帧数)
 *  rel_angle: 相对角度, 正=左转, 负=右转
 * ================================================================ */
static void gyro_turn(float rel_angle)
{
    float start = gyro_get_yaw();
    float last = start;
    float total = 0;  /* 累积已转角度(避开边界回卷) */

    for (int t = 0; t < 500; t++) {
        float cur = gyro_get_yaw();
        float delta = cur - last;
        if (delta >  180.0f) delta -= 360.0f;
        if (delta < -180.0f) delta += 360.0f;
        total += delta;
        last = cur;

        float remaining = rel_angle - total;
        if (remaining > -2.0f && remaining < 2.0f) break;

        float turn = KP_ANGLE * remaining;
        if (turn >  35.0f) turn =  35.0f;
        if (turn < -35.0f) turn = -35.0f;

        if (turn > 2.0f)       motor_set( 25, -25);
        else if (turn < -2.0f) motor_set(-25,  25);
        else break;

        delay_ms(10);
    }
    motor_brake();
    delay_ms(100);
}

/* ================================================================
 *  盲走控制: 陀螺仪锁 seg.target_heading
 * ================================================================ */
static void blind_drive(float target_heading)
{
    (void)target_heading;
    float err = gyro_get_yaw_error();

    float turn = KP_ANGLE * err;
    if (turn >  TURN_LIMIT) turn =  TURN_LIMIT;
    if (turn < -TURN_LIMIT) turn = -TURN_LIMIT;

    /* 软启动: 起步 500ms 内从 30% 渐进到全速 */
    static uint16_t ramp_tick = 0;
    if (g_last_L == 0 && g_last_R == 0) ramp_tick = 0;
    int16_t base = (ramp_tick < 50) ? (20 + (int16_t)(ramp_tick * (SPEED_BLIND - 20) / 50))
                                    : SPEED_BLIND;
    ramp_tick++;

    int16_t L = (int16_t)((float)base + turn);
    int16_t R = (int16_t)((float)base - turn);

    L += 1;  /* 左轮快1pwm */

    if (L < 0)   L = 0;   if (L > 100) L = 100;
    if (R < 0)   R = 0;   if (R > 100) R = 100;

    if (L < 0) L = 0;

    g_last_L = L;
    g_last_R = R;
    motor_set(L, R);
}

/* ================================================================
 *  main
 * ================================================================ */

int main(void)
{
    /* ---- 初始化 ---- */
    SYSCFG_DL_init();
    extra_io_init();

    /* PB10: 外设电源使能 */
    {
        DL_GPIO_initDigitalOutput(IOMUX_PINCM27);
        DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_10);
        DL_GPIO_enableOutput(GPIOB, DL_GPIO_PIN_10);
    }
    delay_ms(100);
    DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_10);
    delay_ms(150);

    lcd_init();

    /* 任务选择 */
    uint8_t task = task_select();
    g_cur_task = task;

    /* 驱动初始化 */
    motor_init();
    sensor_init();
    encoder_init();

    /* 盲走角度 PID (只给角度 P 用, PID 结构仅作角度误差存储) */
    pid_init(&g_pid_angle, KP_ANGLE, 0.0f, 0.0f, -TURN_LIMIT, TURN_LIMIT);
    pid_init(&g_pid_line,  0.6f, 0.0f, 0.0f, -35.0f, 35.0f);  /* 纯P, 无积分 */
    motor_coast();

    /* 陀螺仪初始化 + 校准 */
    lcd_clear(LCD_BLACK);
    lcd_set_cursor(0, 0); lcd_puts("Calibrating...");
    delay_ms(100);
    gyro_init();
    gyro_calibrate(200);
    gyro_calibrate(200);
    lcd_set_cursor(0, 0); lcd_puts("Go!             ");

    /* 设置路径 */
    sm_set_task(task, VERTEX_A);

    /* 初始盲走段: Task3+ 原地转向 target_heading 角度 */
    {
        Segment_t first = sm_get_current_segment();
        if (first.type == SEG_BLIND && task >= 3 && first.target_heading != 0.0f) {
            gyro_turn(first.target_heading);
        }
        sm_sync_start();
        gyro_set_target_yaw(gyro_get_yaw());
    }

    /* 传感器预稳定: 读 500ms 让 LM393 上电稳定 */
    uint8_t dummy[8];
    for (int i = 0; i < 50; i++) {
        sensor_read_all(dummy);
        delay_ms(10);
    }

    /* ================================================================
     *  任务5: 循迹 + 丢线后根据最后感应侧转向找线
     *  左边2路(0,1): 判断直走/丢线, 最后感应→左转
     *  中间4路(2,3,4,5): 循迹
     *  右边2路(6,7): 判断直走/丢线, 最后感应→右转
     * ================================================================ */
    if (task == 5) {
        bt_init();
        encoder_reset_distance();
        lcd_set_cursor(0, 0); lcd_puts("T5 Trace BT");
        uint16_t t5_tick    = 0;
        uint16_t off_ms     = 0;      /* 连续无感应时长(ms) */
        bool     turning    = false;  /* 正在原地旋转 */
        bool     turn_left  = false;  /* 左转/右转 */
        int8_t   last_side  = 0;      /* 最后感应侧: -1=左(0,1), +1=右(6,7), 0=无 */

        while (1) {
            delay_ms(10);
            t5_tick++;

            uint8_t t5_fresh[8];
            sensor_read_all(t5_fresh);
            encoder_update();
            gyro_update(0.01f);

            bool zEL = t5_fresh[0] || t5_fresh[1];  /* 左边2路 */
            bool zM  = t5_fresh[2]||t5_fresh[3]||t5_fresh[4]||t5_fresh[5];  /* 中间4路 */
            bool zER = t5_fresh[6] || t5_fresh[7];  /* 右边2路 */

            /* 记录最后感应侧(仅左右两区) */
            if (zEL) last_side = -1;
            if (zER) last_side =  1;

            /* 中间4路用于判断有无线 */
            if (!zM && !turning) {
                off_ms += 10;
            } else if (zM) {
                off_ms = 0;
            }

            /* 连续100ms无感应 → 根据左右区方向旋转找线 */
            if (off_ms >= 100 && !turning) {
                /* 距离超过200cm → 到达终点, 停止并发送数据 */
                if (encoder_get_distance() > 200.0f) {
                    motor_brake();
                    bt_send_str("DONE\r\n");
                    lcd_set_cursor(1, 0); lcd_puts("FINISH!");
                    while (1) __WFI();
                }
                turning = true;
                turn_left = (last_side == -1);  /* 左区最后→左转, 右区最后→右转 */
            }

            if (turning) {
                /* 原地陀螺仪旋转, 直到中间两路(3,4)同时有黑线 */
                float target = gyro_get_yaw() + (turn_left ? 180.0f : -180.0f);
                gyro_set_target_yaw(target);
                uint8_t t5_s[8];
                bool found = false;
                for (int tt = 0; tt < 300 && !found; tt++) {
                    gyro_update(0.01f);
                    float err = gyro_get_yaw_error();
                    if (err > -1.5f && err < 1.5f) break;
                    float trn = KP_ANGLE * err;
                    if (trn >  15.0f) trn =  15.0f;
                    if (trn < -15.0f) trn = -15.0f;
                    if (trn > 3.0f)       motor_set( 10, -10);
                    else if (trn < -3.0f) motor_set(-10,  10);
                    else break;
                    sensor_read_all(t5_s);
                    if (t5_s[3] && t5_s[4]) found = true;
                    delay_ms(10);
                }
                motor_brake();
                delay_ms(100);
                turning = false;
                off_ms = 0;
            } else if (zM) {
                /* 中间4路循迹: 三区逻辑 */
                bool zL2 = t5_fresh[2] || t5_fresh[3];
                bool zR2 = t5_fresh[4] || t5_fresh[5];
                int16_t steer = 0;
                if (zL2 && !zR2)      steer =  20;
                else if (zR2 && !zL2) steer = -20;
                steer -= (int16_t)((gyro_get_z() - (float)steer * 0.8f) * 0.15f);
                if (steer > 20) steer = 20; if (steer < -20) steer = -20;
                int16_t L = SPEED_T5 + steer;
                int16_t R = SPEED_T5 - steer;
                if (L < 0) L = 0; if (L > 100) L = 100;
                if (R < 0) R = 0; if (R > 100) R = 100;
                g_last_L = L; g_last_R = R;
                motor_set(L, R);
            } else {
                motor_set(SPEED_T5 + 1, SPEED_T5);
                g_last_L = SPEED_T5;
                g_last_R = SPEED_T5;
            }

            if (t5_tick % 20 == 0) {
                lcd_set_cursor(0, 0);
                lcd_puts("D"); lcd_print_int((int32_t)encoder_get_distance());
                lcd_puts("cm  ");
                lcd_set_cursor(1, 0);
                for (int b = 0; b < 8; b++) lcd_putc(t5_fresh[b] ? '1' : '0');
                lcd_puts(turning ? (turn_left ? " L" : " R") : zM ? " M" : " -");
                lcd_puts("  ");
            }

            if (t5_tick % 50 == 0) {
                DL_GPIO_togglePins(LED_G_PORT, LED_G_PIN);
            }
        }
    }

    /* ================================================================
     *  任务6: 纯陀螺仪锁角直走 — 用手推偏后自动回正
     * ================================================================ */
    if (task == 6) {
        lcd_set_cursor(0, 0); lcd_puts("T6 Gyro Lock");
        gyro_set_target_yaw(gyro_get_yaw());  /* 锁当前朝向 */
        uint16_t t6_tick = 0;
        while (1) {
            delay_ms(10);
            t6_tick++;
            gyro_update(0.01f);

            /* 陀螺仪锁角直走 */
            float err = gyro_get_yaw_error();
            float turn = KP_ANGLE * err;
            if (turn >  TURN_LIMIT) turn =  TURN_LIMIT;
            if (turn < -TURN_LIMIT) turn = -TURN_LIMIT;
            int16_t L = SPEED_BLIND + (int16_t)turn;
            int16_t R = SPEED_BLIND - (int16_t)turn;
            L += 1;  /* 左轮快1pwm */
            if (L < 0) L = 0; if (L > 100) L = 100;
            if (R < 0) R = 0; if (R > 100) R = 100;
            motor_set(L, R);

            if (t6_tick % 20 == 0) {
                lcd_set_cursor(0, 0);
                lcd_puts("Y"); lcd_print_int((int32_t)err);
                lcd_puts(" L"); lcd_print_int(L);
                lcd_puts(" R"); lcd_print_int(R);
                lcd_puts("  ");
                lcd_set_cursor(1, 0);
                lcd_puts("E"); lcd_print_int(gyro_get_err());
                lcd_puts(" Yw"); lcd_print_int((int32_t)gyro_get_yaw());
                lcd_puts("  ");
            }
        }
    }

    /* ================================================================
     *  任务7: 灰度+PID 纯循迹测试
     * ================================================================ */
    if (task == 7) {
        lcd_set_cursor(0, 0); lcd_puts("T7 PID Trace");
        uint16_t t7_tick = 0;
        while (1) {
            delay_ms(10);
            t7_tick++;
            gyro_update(0.01f);

            if (sensor_is_line_detected()) {
                line_follow();
            } else {
                motor_set(SPEED_LINE + 1, SPEED_LINE);
                g_last_L = SPEED_LINE;
                g_last_R = SPEED_LINE;
            }

            if (t7_tick % 20 == 0) {
                lcd_set_cursor(0, 0);
                lcd_puts("P"); lcd_print_int(sensor_get_position());
                lcd_puts(" L"); lcd_print_int(g_last_L);
                lcd_puts(" R"); lcd_print_int(g_last_R);
                lcd_puts("  ");
                lcd_set_cursor(1, 0);
                /* 8路灰度值简略显示 */
                uint16_t g[8]; sensor_get_gray(g);
                for (int b = 0; b < 8; b++) {
                    lcd_print_int(g[b] / 100); lcd_putc(' ');
                }
                lcd_puts(" ");
            }
        }
    }

    /* ================================================================
     *  任务8: 蓝牙发送 "1"
     * ================================================================ */
    if (task == 8) {
        bt_init();
        lcd_set_cursor(0, 0); lcd_puts("T8 BT Send 1");
        while (1) {
            bt_send_str("1\r\n");
            delay_ms(500);
        }
    }

    /* ================================================================
     *  任务9: 视觉寻球 → 中心对准 → 直走 → 球消失 → 发送数据
     * ================================================================ */
    if (task == 9) {
        bt_init();
        vision_init();

        lcd_set_cursor(0, 0); lcd_puts("T9 Vision");
        gyro_update(0.01f);

        /* 等待视觉检测到球 */
        while (!g_ball_valid) {
            lcd_set_cursor(1, 0);
            lcd_puts("X"); lcd_print_int((int32_t)g_ball_x);
            lcd_puts(" Y"); lcd_print_int((int32_t)g_ball_y);
            lcd_puts("     ");
            delay_ms(100);
        }
        lcd_set_cursor(1, 0); lcd_puts("FOUND!      ");

        /* 原地慢速旋转对准球心 */
        while (1) {
            int16_t dx = (int16_t)g_ball_x - 320;
            lcd_set_cursor(1, 0);
            lcd_puts("dx"); lcd_print_int((int32_t)dx);
            lcd_puts("   ");
            if (dx > -20 && dx < 20) break;
            if (dx < 0) motor_set( 10, -10);
            else        motor_set(-10,  10);
            delay_ms(10);
        }
        motor_brake();
        delay_ms(200);

        /* 直走靠近球, 偏离中线则停下重调 */
        uint16_t lost_ms = 0;
        while (1) {
            /* 每帧先清零, 等下个ISR中断重新置1 */
            g_ball_valid = 0;
            delay_ms(10);

            int16_t dx = (int16_t)g_ball_x - 320;

            /* 球消失50ms确认 */
            if (!g_ball_valid) {
                lost_ms += 10;
                if (lost_ms >= 500) break;
            } else {
                lost_ms = 0;
            }

            /* 偏离太大 → 停车重调 */
            if (dx < -50 || dx > 50) {
                motor_brake();
                delay_ms(100);
                uint16_t sweep_cnt   = 0;
                uint16_t lost_adj_ms  = 0;
                uint16_t adj_total_ms = 0;
                uint16_t stuck_ms     = 0;
                int16_t  last_dx      = 999;
                uint8_t  last_valid   = 2;
                bool sweep_left = true;
                bool sweeping   = false;
                bool backing    = false;
                uint8_t adj_valid;
                while (1) {
                    g_ball_valid = 0;
                    delay_ms(10);
                    adj_valid = g_ball_valid;
                    dx = (int16_t)g_ball_x - 320;
                    adj_total_ms += 10;
                    /* 检测卡住: dx和valid都不变 */
                    if (dx == last_dx && adj_valid == last_valid)
                        stuck_ms += 10;
                    else { stuck_ms = 0; last_dx = dx; last_valid = adj_valid; }
                    /* 卡住超500ms → 强制左右各转30ms */
                    if (stuck_ms >= 500) {
                        motor_set( 20, -20); delay_ms(300);
                        motor_set(-20,  20); delay_ms(300);
                        stuck_ms = 0;
                    }
                    lcd_set_cursor(1, 0);
                    lcd_puts(backing ? "BAK" : sweeping ? "SWP" : "ADJ");
                    lcd_print_int((int32_t)dx);
                    lcd_puts("   ");
                    /* 有球且居中 → 退出 */
                    if (adj_valid && dx > -30 && dx < 30) break;
                    /* 持续2s无球 → 后退 */
                    if (!adj_valid && adj_total_ms > 2000) backing = true;
                    if (adj_valid && backing) {
                        /* 重新看到球后再退300ms */
                        uint16_t bak_extra = 0;
                        while (bak_extra < 300) {
                            motor_set(-15, -15);
                            delay_ms(10);
                            bak_extra += 10;
                        }
                        backing = false;
                        adj_total_ms = 0;
                    } else if (adj_valid) {
                        adj_total_ms = 0;
                    }
                    /* 消抖: 球连续消失300ms → 左右扫 */
                    if (!adj_valid) {
                        lost_adj_ms += 10;
                        if (lost_adj_ms >= 300 && !backing) sweeping = true;
                    } else {
                        lost_adj_ms = 0;
                        sweeping = false;
                    }
                    if (backing) {
                        motor_set(-15, -15);
                    } else if (sweeping) {
                        sweep_cnt++;
                        if (sweep_cnt >= 30) { sweep_left = !sweep_left; sweep_cnt = 0; }
                        if (sweep_left) motor_set( 10, -10);
                        else            motor_set(-10,  10);
                    } else {
                        /* 有球: 向球方向转 */
                        if (dx < 0) motor_set( 10, -10);
                        else        motor_set(-10,  10);
                    }
                }
                motor_brake();
                delay_ms(200);
            }

            motor_set(15, 15);  /* 直走 */
            lcd_set_cursor(1, 0);
            lcd_puts("X"); lcd_print_int((int32_t)g_ball_x);
            lcd_puts("   ");
        }

        /* 球消失 */
        motor_brake();
        bt_send_str("done\r\n");
        lcd_set_cursor(1, 0); lcd_puts("DONE!");
        while (1) __WFI();
    }

    /* ================================================================
     *  任务10: 原地旋转 180°
     * ================================================================ */
    if (task == 10) {
        lcd_set_cursor(0, 0); lcd_puts("T10 Y:");
        while (1) {
            float y = gyro_get_yaw();
            lcd_set_cursor(0, 6);
            lcd_print_int((int32_t)y);
            lcd_puts("   ");
            if (y > 175.0f || y < -175.0f) break;
            motor_set(-12, 12);   /* 慢速右转 */
            delay_ms(20);
        }
        motor_brake();
        lcd_set_cursor(1, 0); lcd_puts("DONE!");
        while (1) __WFI();
    }

    /* ---- 任务1~4 主循环 ---- */
    uint16_t  tick = 0;
    Segment_t seg;

    while (1) {
        delay_ms(10);
        tick++;

        /* 1. 刷新传感器 (更新 g_sensor_buf) */
        uint8_t _fresh[8];
        sensor_read_all(_fresh);

        /* 2. 刷新编码器里程 */
        encoder_update();
        float dist = encoder_get_distance();

        /* 3. 刷新陀螺仪 */
        gyro_update(0.01f);

        /* 4. 获取当前路径段 */
        seg = sm_get_current_segment();
        if (seg.type == SEG_DONE) {
            motor_coast();
            beep(500);
            lcd_set_cursor(0, 0);
            lcd_puts("DONE!          ");
            while (1) __WFI();
        }

        /* 5. 顶点检测: 入弯需≥2路, 出弯≥1路 */
        bool line_now = sensor_is_line_detected();
        bool line_check = (seg.type == SEG_BLIND) ? sensor_is_line_detected2() : line_now;
        Vertex_t v = sm_check_vertex(dist, line_check, seg.target_distance);

        /* 入弯(盲走→循迹)挂起: 先直走再停 */
        static bool   v_pending  = false;
        static float  v_start    = 0.0f;
        static float  v_pend_cm  = 4.0f;
        static Vertex_t v_saved  = VERTEX_NONE;

        if (v != VERTEX_NONE && !v_pending) {
            v_saved = v;                              /* 始终保存 */
            if (seg.type == SEG_BLIND && task == 3) {
                v_pending = true;
                v_start   = dist;
                v_pend_cm = 5.0f;  /* Task3入弯5cm */
            }
        }

        if (v_pending && (dist - v_start) < v_pend_cm) {
            motor_set(SPEED_LINE + 1, SPEED_LINE);
            g_last_L = SPEED_LINE;
            g_last_R = SPEED_LINE;
            goto lcd_update;
        }

        if ((v_pending && (dist - v_start) >= v_pend_cm) || (v != VERTEX_NONE && !v_pending)) {
            /* 入弯5cm到位 或 出弯即刻: 执行顶点处理 */
            bool was_line = v_pending ? false : (seg.type != SEG_BLIND);
            if (v_pending) { v = v_saved; v_pending = false; }
            /* Task3出弯: 直走5cm再刹车 */
            if (task == 3 && (v_saved == VERTEX_B || v_saved == VERTEX_A) && was_line) {
                float s0 = encoder_get_distance();
                while ((encoder_get_distance() - s0) < 5.0f) {
                    motor_set(SPEED_LINE + 1, SPEED_LINE);
                    encoder_update();
                    delay_ms(10);
                }
            }
            motor_brake();
            beep(100);                /* 声光提示到达顶点 */
            for (int i = 0; i < 30; i++) {
                gyro_update(0.01f);     /* 保持 BMI160 通信活跃 */
                gyro_update_bias();     /* 静止时更新零偏 */
                delay_ms(10);
            }
            sm_vertex_reached(v_saved);
            line_follow_reset();
            pid_reset(&g_pid_angle);
            Segment_t next = sm_get_current_segment();

            /* 出弯: 陀螺仪控转向 */
            if (was_line) {
                if (task == 3 && v_saved == VERTEX_A)
                    gyro_turn(-15.0f);   /* 第二次出弯右转 */
                else if (task == 3)
                    gyro_turn( 15.0f);   /* 第一次出弯左转 */
                else
                    gyro_turn( -9.5f);   /* Task2 右转 */
            }
            /* Task3 盲走入弯: 陀螺仪控转向 */
            if (!was_line && task == 3) {
                if (v_saved == VERTEX_D)
                    gyro_turn(-22.0f);   /* 第二次入弯右转 */
                else
                    gyro_turn( 24.5f);   /* 第一次入弯左转 */
            }
            /* Task3 盲走起始偏角: 陀螺仪控 */
            if (next.type == SEG_BLIND && task >= 3 && next.target_heading != 0.0f) {
                gyro_turn(next.target_heading);
            }
            sm_sync_start();
            /* 盲走段: 锁当前朝向 */
            if (next.type == SEG_BLIND) {
                gyro_set_target_yaw(gyro_get_yaw());
            }
            continue;
        }

        /* 6. 控制: 有线→循迹, 无线→盲走 */
        static bool last_line_state = false;
        if (line_now) {
            /* 刚从盲走切到循迹: 刷新 PID, 平滑入线 */
            if (!last_line_state) line_follow_reset();
            line_follow();
        } else {
            blind_drive(seg.target_heading);
        }
        last_line_state = line_now;

lcd_update:
        /* 7. LCD 刷新 (每 100ms) */
        if (tick % 10 == 0) {
            lcd_set_cursor(0, 0);
            lcd_puts("D"); lcd_print_int((int32_t)dist);
            lcd_puts(" Y"); lcd_print_int((int32_t)gyro_get_yaw_error());
            lcd_puts("  ");
            lcd_set_cursor(1, 0);
            lcd_puts(line_now ? "LINE" : "BLND");
            lcd_puts(" P"); lcd_print_int(sensor_get_position());
            lcd_puts(" "); lcd_print_int(g_last_L);
            lcd_puts("/"); lcd_print_int(g_last_R);
            lcd_puts("  ");
        }

        /* 8. 停车时零偏自动校准 */
        static uint16_t cal_tick = 0;
        if (g_last_L == 0 && g_last_R == 0) {
            cal_tick++;
        } else {
            cal_tick = 0;
        }
        if (cal_tick >= 100) {
            gyro_calibrate(20);
            cal_tick = 0;
        }

        /* 9. 心跳 LED */
        if (tick % 50 == 0) {
            DL_GPIO_togglePins(LED_G_PORT, LED_G_PIN);
        }
    }
}
