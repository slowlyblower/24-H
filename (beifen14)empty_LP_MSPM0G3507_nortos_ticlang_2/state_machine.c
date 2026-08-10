/*
 * state_machine.c - 路径状态机实现
 *
 * 场地几何 (3-4-5三角形):
 *   A-B 水平距离 60cm
 *   C-D 水平距离 60cm
 *   A-C 垂直距离 80cm  (对角线 ≈100cm)
 *   B-D 垂直距离 80cm  (对角线 ≈100cm)
 *   弧线 BC ≈125.7cm (右半圆弧, 半径40cm)
 *   弧线 DA ≈125.7cm (左半圆弧, 半径40cm)
 *
 * 顶点检测规则:
 *   盲走段 → 传感器检测到黑线 = 到达顶点
 *   循迹段 → 传感器检测不到黑线 = 到达顶点
 *   距离需在目标距离的 ±10% 以内 (或超时 1.4x 强制到达)
 */

#include "state_machine.h"
#include "sensor.h"
#include "encoder.h"
#include <math.h>

/* ---- 场地参数 (cm) ---- */
#define DIST_AB     60.0f
#define DIST_CD     60.0f
#define DIST_AC     80.0f
#define DIST_BD     80.0f
#define DIST_DIAG   100.0f    /* sqrt(60²+80²) */
#define DIST_ARC    125.7f

/* ---- 顶点检测容差 ---- */

/* ---- 内部状态 ---- */
static uint8_t   g_task      = 0;
static uint8_t   g_seg_idx   = 0;
static uint8_t   g_lap       = 0;
static uint8_t   g_lap_total = 1;
static Vertex_t  g_current_v = VERTEX_NONE;
static Segment_t g_segments[12];
static uint8_t   g_seg_count = 0;
static float     g_seg_start = 0.0f;    /* 当前段起始距禈 */

/* ================================================================
 *  预定义路径 (按任务)
 * ================================================================ */

/* 任务1: A→B (盲走 60cm) */
static const Segment_t path_task1[] = {
    {SEG_BLIND,     0.0f,   DIST_AB,   VERTEX_B},
    {SEG_DONE,      0.0f,   0.0f,      VERTEX_NONE}
};

/* 任务2: A→B(盲走)→弧线BC(循迹)→C→D(盲走)→弧线DA(循迹)→A */
static const Segment_t path_task2[] = {
    {SEG_BLIND,     0.0f,   DIST_AB,   VERTEX_B},
    {SEG_LINE_BC,   0.0f,   DIST_ARC,  VERTEX_C},
    {SEG_BLIND,     90.0f,  DIST_CD,   VERTEX_D},
    {SEG_LINE_DA,   0.0f,   DIST_ARC,  VERTEX_A},
    {SEG_DONE,      0.0f,   0.0f,      VERTEX_NONE}
};

/* 任务3/4: A→C(盲走对角)→弧线CB(循迹)→B→D(盲走)→弧线DA(循迹)→A */
static const Segment_t path_task3[] = {
    {SEG_BLIND,     -24.5f,  DIST_DIAG,  VERTEX_C},   /* A→C 右转 */
    {SEG_LINE_CB,    0.0f,   DIST_ARC,   VERTEX_B},
    {SEG_BLIND,      17.5f,  DIST_BD,    VERTEX_D},   /* B→D 左转 */
    {SEG_LINE_DA,    0.0f,   DIST_ARC,   VERTEX_A},
    {SEG_DONE,       0.0f,   0.0f,       VERTEX_NONE}
};

/* ================================================================
 *  API 实现
 * ================================================================ */

void sm_set_task(uint8_t task_id, Vertex_t start)
{
    g_task      = task_id;
    g_seg_idx   = 0;
    g_lap       = 0;
    g_current_v = start;
    g_lap_total = (task_id == 4) ? 4 : 1;
    g_seg_start = encoder_get_distance();
    const Segment_t *src;
    uint8_t cnt;

    switch (task_id) {
        case 1:
            src = path_task1; cnt = 1; break;
        case 2:
            src = path_task2; cnt = 4; break;
        case 3:
        case 4:
            src = path_task3; cnt = 4; break;
        default:
            return;
    }

    for (uint8_t i = 0; i < cnt; i++) {
        g_segments[i] = src[i];
    }
    g_seg_count = cnt;
}

Segment_t sm_get_current_segment(void)
{
    if (g_seg_idx >= g_seg_count) {
        Segment_t done = {SEG_DONE, 0.0f, 0.0f, VERTEX_NONE};
        return done;
    }
    return g_segments[g_seg_idx];
}

uint8_t sm_get_task(void)  { return g_task; }
bool    sm_is_done(void)   { return g_seg_idx >= g_seg_count; }
uint8_t sm_get_lap(void)   { return g_lap; }

void sm_sync_start(void)
{
    g_seg_start = encoder_get_distance();
}

/*
 * 顶点检测:
 *   盲走段: 距禮>5cm 且检测到线 → 到达
 *   循迹段: 距禮过半 且全白 → 到达
 *   超时保护: 距禮 > 目标*1.4 强制到达
 */
Vertex_t sm_check_vertex(float distance, bool line_detected,
                         float target_dist)
{
    float seg_dist = distance - g_seg_start;  /* 当前段已走距禈 */
    SegType_t type = g_segments[g_seg_idx].type;
    bool arrived = false;

    if (type == SEG_BLIND) {
        if (line_detected && seg_dist > target_dist * 0.45f) arrived = true;
    } else if (type == SEG_LINE_BC || type == SEG_LINE_DA || type == SEG_LINE_CB) {
        if (!line_detected && seg_dist > target_dist * 0.25f) arrived = true;
    }

    if (seg_dist > target_dist * 1.4f) {
        arrived = true;
    }

    if (arrived) {
        return g_segments[g_seg_idx].destination;
    }
    return VERTEX_NONE;
}

/* 到达顶点: 切换下一段, 任务3/4 处理多圈 */
void sm_vertex_reached(Vertex_t v)
{
    g_current_v = v;
    g_seg_idx++;
    g_seg_start = encoder_get_distance();  /* 记录新段起点 */

    if (g_seg_idx >= g_seg_count) {
        g_lap++;
        if (g_lap < g_lap_total) {
            g_seg_idx = 0;
            g_seg_start = encoder_get_distance();
        }
    }
}

/* 盲走纠偏: 走了预期 1.3 倍还没到, 触发扇形搜索 (当前版本已不需要) */
bool sm_needs_recovery(float distance, float target_dist)
{
    Segment_t seg = g_segments[g_seg_idx];
    if (seg.type != SEG_BLIND) return false;
    return (distance > target_dist * 1.3f);
}
