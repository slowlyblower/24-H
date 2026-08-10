/*
 * state_machine.h - 路径状态机 + 任务管理
 *
 * 顶点: A(左上) B(右上) C(右下) D(左下)
 *
 * 任务1: A→B (盲走)
 * 任务2: A→B(盲走)→弧线BC(循迹)→C→D(盲走)→弧线DA(循迹)→A
 * 任务3: A→C(盲走对角)→弧线CB(循迹)→B→D(盲走)→弧线DA(循迹)→A
 * 任务4: 同任务3, 连跑4圈
 * 任务5: 纯循迹测试 (无穷线段)
 *
 * 路径段定义:
 *   SEG_BLIND    - 盲走段 (BMI160锁角)
 *   SEG_LINE_BC  - 循迹弧线 B→C
 *   SEG_LINE_DA  - 循迹弧线 D→A
 *   SEG_LINE_CB  - 循迹弧线 C→B (任务3用)
 */

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stdint.h>
#include <stdbool.h>

/* 顶点 */
typedef enum {
    VERTEX_NONE = 0,
    VERTEX_A, VERTEX_B, VERTEX_C, VERTEX_D
} Vertex_t;

/* 路径段类型 */
typedef enum {
    SEG_BLIND,       /* 盲走(角度锁定) */
    SEG_LINE_BC,     /* 循迹弧线 B→C */
    SEG_LINE_DA,     /* 循迹弧线 D→A */
    SEG_LINE_CB,     /* 循迹弧线 C→B (反向) */
    SEG_DONE         /* 完成 */
} SegType_t;

/* 路径段 */
typedef struct {
    SegType_t type;
    float     target_heading;   /* 盲走时的锁角(°) */
    float     target_distance;  /* 盲走时的期望里程(cm) */
    Vertex_t  destination;      /* 到达顶点 */
} Segment_t;

/* ---- API ---- */

/* 设置当前任务 (1~4), 初始顶点 */
void sm_set_task(uint8_t task_id, Vertex_t start);

/* 获取当前路径段 */
Segment_t sm_get_current_segment(void);

/* 获取当前任务 */
uint8_t sm_get_task(void);

/* 检查是否到达顶点 (基于里程+传感器状态) */
/* 输入: 当前里程(cm), 传感器检测到线, 目标距离 */
/* 返回: 到达的顶点 (VERTEX_NONE=未到) */
Vertex_t sm_check_vertex(float distance, bool line_detected,
                         float target_dist);

/* 到达顶点后: 蜂鸣+LED, 切换到下一段 */
void sm_vertex_reached(Vertex_t v);

/* 同步段起始距禈 (原地转向后调用) */
void sm_sync_start(void);

/* 任务是否完成 */
bool sm_is_done(void);

/* 当前圈数 (任务4) */
uint8_t sm_get_lap(void);

/* 盲走纠偏: 走了预期1.3倍还没到, 触发扇形扫描 */
bool sm_needs_recovery(float distance, float target_dist);

#endif
