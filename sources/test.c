/**
 * @file test.c
 * @brief EventScheduling 公共 API 测试套件
 *
 * 测试覆盖：
 *   - Create / Destroy 生命周期
 *   - TaskRegister / TaskUnRegister 注册与注销
 *   - setEventToTask 事件投递与边界校验
 *   - TmosPoll 协作式轮询调度
 *   - 多实例独立性
 *   - NULL / 无效参数防御
 */

#include <stdio.h>
#include <stdlib.h>
#include "EventSchedul.h"

/* ==================== 简易测试断言 ==================== */

static int g_pass = 0;
static int g_fail = 0;

#define TEST_ASSERT(cond, fmt, ...) do {                    \
    if (cond) {                                              \
        g_pass++;                                            \
        printf("  [PASS] " fmt "\n", ##__VA_ARGS__);         \
    } else {                                                 \
        g_fail++;                                            \
        printf("  [FAIL] " fmt "\n", ##__VA_ARGS__);         \
    }                                                        \
} while (0)

/* ==================== 全局回调标记（用于验证回调是否被触发） ==================== */

static EventSchedul_EventId g_last_evt  = 0;
static void*                g_last_arg  = NULL;
static int                  g_callbacks_fired = 0;

static void reset_callback_trace(void) {
    g_last_evt  = 0;
    g_last_arg  = NULL;
    g_callbacks_fired = 0;
}

/* 测试用任务回调 */
static void test_task_cb(EventSchedul_EventId recvEvt, void* arg) {
    g_last_evt  = recvEvt;
    g_last_arg  = arg;
    g_callbacks_fired++;
}

static void test_task_cb2(EventSchedul_EventId recvEvt, void* arg) {
    (void)recvEvt;
    (void)arg;
    /* 第二个回调，仅计数 */
    g_callbacks_fired++;
}

/* dummy 空回调（仅占位，不做任何事） */
static void dummy_cb(EventSchedul_EventId recvEvt, void* arg) {
    (void)recvEvt;
    (void)arg;
}

/* ==================== 测试用例 ==================== */

/* --- 1. 生命周期：Create / Destroy --- */
void test_create_destroy(void) {
    printf("--- test_create_destroy ---\n");

    /* 正常创建 */
    EventSchedul_Context* ctx = EventSchedul_Create(
        &(EventSchedul_Allocator){malloc, free});
    TEST_ASSERT(ctx != NULL, "Create with valid allocator returns non-NULL");

    /* NULL 分配器 */
    EventSchedul_Context* ctx2 = EventSchedul_Create(NULL);
    TEST_ASSERT(ctx2 == NULL, "Create(NULL) returns NULL");

    /* malloc=NULL 的分配器 */
    EventSchedul_Context* ctx3 = EventSchedul_Create(
        &(EventSchedul_Allocator){NULL, free});
    TEST_ASSERT(ctx3 == NULL, "Create with malloc=NULL returns NULL");

    /* free=NULL 的分配器 */
    EventSchedul_Context* ctx4 = EventSchedul_Create(
        &(EventSchedul_Allocator){malloc, NULL});
    TEST_ASSERT(ctx4 == NULL, "Create with free=NULL returns NULL");

    /* Destroy(NULL) 不崩溃 */
    EventSchedul_Destroy(NULL);
    TEST_ASSERT(1, "Destroy(NULL) does not crash");

    /* 正常销毁 */
    EventSchedul_Destroy(ctx);
    TEST_ASSERT(1, "Destroy valid context");
}

/* --- 2. 任务注册 --- */
void test_task_register(void) {
    printf("--- test_task_register ---\n");

    EventSchedul_Context* ctx = EventSchedul_Create(
        &(EventSchedul_Allocator){malloc, free});
    TEST_ASSERT(ctx != NULL, "Create succeeded");

    /* NULL ctx */
    EventSchedul_TaskNode* h = EventSchedul_TaskRegister(NULL, NULL);
    TEST_ASSERT(h == NULL, "TaskRegister(NULL,NULL) returns NULL");

    /* 有效注册 */
    EventSchedul_TaskNode cfg = { .pTaskFunc = test_task_cb, .pTaskFuncArg = NULL,
        .info = { .eventStart = 0x0001, .eventEnd = 0x000f } };
    h = EventSchedul_TaskRegister(ctx, &cfg);
    TEST_ASSERT(h != NULL, "TaskRegister valid cfg returns non-NULL");
    TEST_ASSERT(h->pTaskFunc == test_task_cb, "handle points to registered func");
    TEST_ASSERT(h->info.taskId > EVTSCHEDUL_INIT_TASK_ID, "taskId > INIT_TASK_ID");

    /* pTaskFunc=NULL 不应注册 */
    EventSchedul_TaskNode bad_cfg = { .pTaskFunc = NULL, .pTaskFuncArg = NULL,
        .info = { .eventStart = 0, .eventEnd = 0 } };
    EventSchedul_TaskNode* h2 = EventSchedul_TaskRegister(ctx, &bad_cfg);
    TEST_ASSERT(h2 == NULL, "TaskRegister with pTaskFunc=NULL returns NULL");

    /* cfg=NULL */
    EventSchedul_TaskNode* h3 = EventSchedul_TaskRegister(ctx, NULL);
    TEST_ASSERT(h3 == NULL, "TaskRegister(cfg=NULL) returns NULL");

    EventSchedul_Destroy(ctx);
}

/* --- 3. 静态模式：填满任务池 --- */
void test_register_full(void) {
    printf("--- test_register_full ---\n");

    EventSchedul_Context* ctx = EventSchedul_Create(
        &(EventSchedul_Allocator){malloc, free});
    TEST_ASSERT(ctx != NULL, "Create succeeded");

    EventSchedul_TaskNode cfg = { .pTaskFunc = dummy_cb, .pTaskFuncArg = NULL,
        .info = { .eventStart = 0x0001, .eventEnd = 0x00ff } };

    EventSchedul_TaskNode* handles[EVTSCHEDUL_TASKS_MAX];
    int i;
    for (i = 0; i < EVTSCHEDUL_TASKS_MAX; i++) {
        handles[i] = EventSchedul_TaskRegister(ctx, &cfg);
        if (handles[i] == NULL)
            break;
    }
    TEST_ASSERT(i == EVTSCHEDUL_TASKS_MAX,
        "Registered %d tasks (max=%d)", i, EVTSCHEDUL_TASKS_MAX);

    /* 第 EVTSCHEDUL_TASKS_MAX+1 次应失败 */
    EventSchedul_TaskNode* overflow = EventSchedul_TaskRegister(ctx, &cfg);
    TEST_ASSERT(overflow == NULL, "Register beyond max returns NULL");

    EventSchedul_Destroy(ctx);
}

/* --- 4. 任务注销 --- */
void test_task_unregister(void) {
    printf("--- test_task_unregister ---\n");

    EventSchedul_Context* ctx = EventSchedul_Create(
        &(EventSchedul_Allocator){malloc, free});
    TEST_ASSERT(ctx != NULL, "Create succeeded");

    /* NULL 参数 */
    TEST_ASSERT(EventSchedul_TaskUnRegister(NULL, NULL) == EVTSCHEDUL_ERR_ARG,
        "TaskUnRegister(NULL,NULL) returns ERR_ARG");

    EventSchedul_TaskNode cfg = { .pTaskFunc = test_task_cb, .pTaskFuncArg = NULL,
        .info = { .eventStart = 0x0001, .eventEnd = 0x00ff } };
    EventSchedul_TaskNode* h = EventSchedul_TaskRegister(ctx, &cfg);
    TEST_ASSERT(h != NULL, "Task registered");

    /* 正常注销 */
    int ret = EventSchedul_TaskUnRegister(ctx, h);
    TEST_ASSERT(ret == EVTSCHEDUL_OK, "TaskUnRegister returns OK");

    /* 静态模式：注销后槽位可复用（再注册能成功） */
    EventSchedul_TaskNode* h2 = EventSchedul_TaskRegister(ctx, &cfg);
    TEST_ASSERT(h2 != NULL, "Re-register after unregister succeeds");

    EventSchedul_Destroy(ctx);
}

/* --- 5. setEventToTask 事件投递与校验 --- */
void test_set_event(void) {
    printf("--- test_set_event ---\n");

    EventSchedul_Context* ctx = EventSchedul_Create(
        &(EventSchedul_Allocator){malloc, free});
    TEST_ASSERT(ctx != NULL, "Create succeeded");

    EventSchedul_TaskNode cfg = { .pTaskFunc = test_task_cb, .pTaskFuncArg = (void*)0xDEAD,
        .info = { .eventStart = 0x0010, .eventEnd = 0x0020 } };
    EventSchedul_TaskNode* h = EventSchedul_TaskRegister(ctx, &cfg);
    TEST_ASSERT(h != NULL, "Task registered (events [0x0010, 0x0020))");

    /* NULL 参数 */
    TEST_ASSERT(EventSchedul_setEventToTask(NULL, h, 0x0015) == EVTSCHEDUL_ERR_ARG,
        "setEvent(NULL,...) returns ERR_ARG");
    TEST_ASSERT(EventSchedul_setEventToTask(ctx, NULL, 0x0015) == EVTSCHEDUL_ERR_ARG,
        "setEvent(ctx,NULL,...) returns ERR_ARG");

    /* 无效目标 */
    EventSchedul_TaskNode fake = {0};
    TEST_ASSERT(EventSchedul_setEventToTask(ctx, &fake, 0x0015) == EVTSCHEDUL_ERR_TASK,
        "setEvent with unregistered target returns ERR_TASK");

    /* 合法事件：start=0x0010, end=0x0020，合法区间 [0x0010, 0x001f] */
    TEST_ASSERT(EventSchedul_setEventToTask(ctx, h, 0x0010) == EVTSCHEDUL_OK,
        "setEvent 0x0010 (==start) returns OK");
    TEST_ASSERT(EventSchedul_setEventToTask(ctx, h, 0x0015) == EVTSCHEDUL_OK,
        "setEvent 0x0015 (mid-range) returns OK");
    TEST_ASSERT(EventSchedul_setEventToTask(ctx, h, 0x001f) == EVTSCHEDUL_OK,
        "setEvent 0x001f (==end-1) returns OK");

    /* 非法事件（区间外） */
    TEST_ASSERT(EventSchedul_setEventToTask(ctx, h, 0x000f) == EVTSCHEDUL_ERR_EVENT,
        "setEvent 0x000f (below start) returns ERR_EVENT");
    TEST_ASSERT(EventSchedul_setEventToTask(ctx, h, 0x0020) == EVTSCHEDUL_ERR_EVENT,
        "setEvent 0x0020 (==end) returns ERR_EVENT");

    /* INVALID_EVT 和 INIT_EVT 被拒绝 */
    TEST_ASSERT(EventSchedul_setEventToTask(ctx, h, EVTSCHEDUL_INVALID_EVT) == EVTSCHEDUL_ERR_ARG,
        "setEvent INVALID_EVT returns ERR_ARG");
    TEST_ASSERT(EventSchedul_setEventToTask(ctx, h, EVTSCHEDUL_INIT_EVT) == EVTSCHEDUL_ERR_ARG,
        "setEvent INIT_EVT returns ERR_ARG");

    EventSchedul_Destroy(ctx);
}

/* --- 6. TmosPoll 协作式轮询 --- */
void test_tmos_poll(void) {
    printf("--- test_tmos_poll ---\n");

    EventSchedul_Context* ctx = EventSchedul_Create(
        &(EventSchedul_Allocator){malloc, free});
    TEST_ASSERT(ctx != NULL, "Create succeeded");

    reset_callback_trace();

    EventSchedul_TaskNode cfg = { .pTaskFunc = test_task_cb,
        .pTaskFuncArg = (void*)0xBEEF,
        .info = { .eventStart = 0x0001, .eventEnd = 0x0100 } };
    EventSchedul_TaskNode* h = EventSchedul_TaskRegister(ctx, &cfg);
    TEST_ASSERT(h != NULL, "Task registered");

    /* 队列空时应返回 NOTHING */
    int ret = EventSchedul_TmosPoll(ctx);
    TEST_ASSERT(ret == EVTSCHEDUL_ERR_NOTHING,
        "TmosPoll on empty queue returns ERR_NOTHING");

    /* 投递一个事件 */
    ret = EventSchedul_setEventToTask(ctx, h, 0x0042);
    TEST_ASSERT(ret == EVTSCHEDUL_OK, "setEvent 0x0042 succeeded");

    /* Poll 应处理该事件 */
    ret = EventSchedul_TmosPoll(ctx);
    TEST_ASSERT(ret == EVTSCHEDUL_OK, "TmosPoll returns OK after event pushed");
    TEST_ASSERT(g_last_evt == 0x0042,
        "Callback received event 0x0042 (got 0x%04x)", g_last_evt);
    TEST_ASSERT(g_last_arg == (void*)0xBEEF,
        "Callback received arg 0xBEEF (got %p)", g_last_arg);
    TEST_ASSERT(g_callbacks_fired == 1,
        "Exactly 1 callback fired (got %d)", g_callbacks_fired);

    /* 队列再次为空 */
    ret = EventSchedul_TmosPoll(ctx);
    TEST_ASSERT(ret == EVTSCHEDUL_ERR_NOTHING,
        "TmosPoll after drain returns ERR_NOTHING");

    /* NULL ctx */
    TEST_ASSERT(EventSchedul_TmosPoll(NULL) == EVTSCHEDUL_ERR_ARG,
        "TmosPoll(NULL) returns ERR_ARG");

    EventSchedul_Destroy(ctx);
}

/* --- 7. 多实例独立性 --- */
void test_multi_instance(void) {
    printf("--- test_multi_instance ---\n");

    EventSchedul_Context* ctx1 = EventSchedul_Create(
        &(EventSchedul_Allocator){malloc, free});
    EventSchedul_Context* ctx2 = EventSchedul_Create(
        &(EventSchedul_Allocator){malloc, free});
    TEST_ASSERT(ctx1 != NULL && ctx2 != NULL, "Two instances created");
    TEST_ASSERT(ctx1 != ctx2, "Instances have different addresses");

    /* ctx1 注册任务 A, ctx2 注册任务 B */
    EventSchedul_TaskNode cfgA = { .pTaskFunc = test_task_cb,
        .pTaskFuncArg = (void*)0xAAAA,
        .info = { .eventStart = 0x0001, .eventEnd = 0x0100 } };
    EventSchedul_TaskNode cfgB = { .pTaskFunc = test_task_cb2,
        .pTaskFuncArg = (void*)0xBBBB,
        .info = { .eventStart = 0x1000, .eventEnd = 0x2000 } };

    EventSchedul_TaskNode* hA = EventSchedul_TaskRegister(ctx1, &cfgA);
    EventSchedul_TaskNode* hB = EventSchedul_TaskRegister(ctx2, &cfgB);
    TEST_ASSERT(hA != NULL && hB != NULL, "Both tasks registered");
    TEST_ASSERT(hA != hB, "Task handles differ between instances");

    /* ctx1 投事件不会影响 ctx2 */
    reset_callback_trace();
    EventSchedul_setEventToTask(ctx1, hA, 0x0055);
    int ret = EventSchedul_TmosPoll(ctx1);
    TEST_ASSERT(ret == EVTSCHEDUL_OK, "ctx1 TmosPoll returns OK");
    TEST_ASSERT(g_last_evt == 0x0055 && g_last_arg == (void*)0xAAAA,
        "ctx1 callback fired with correct args");

    /* ctx2 队列仍为空 */
    ret = EventSchedul_TmosPoll(ctx2);
    TEST_ASSERT(ret == EVTSCHEDUL_ERR_NOTHING,
        "ctx2 queue still empty (isolation verified)");

    /* ctx2 投自己的事件 */
    reset_callback_trace();
    EventSchedul_setEventToTask(ctx2, hB, 0x1234);
    ret = EventSchedul_TmosPoll(ctx2);
    TEST_ASSERT(ret == EVTSCHEDUL_OK, "ctx2 TmosPoll processes own event");
    TEST_ASSERT(g_callbacks_fired == 1,
        "ctx2 callback fired (isolation verified)");

    /* ctx1 独立销毁，ctx2 不受影响 */
    EventSchedul_Destroy(ctx1);
    ret = EventSchedul_TmosPoll(ctx2);
    TEST_ASSERT(ret == EVTSCHEDUL_ERR_NOTHING,
        "ctx2 still functional after ctx1 destroyed");

    EventSchedul_Destroy(ctx2);
}

/* --- 8. 事件区间回绕（start > end 表示中间区间未注册） --- */
void test_event_range_wrap(void) {
    printf("--- test_event_range_wrap ---\n");

    EventSchedul_Context* ctx = EventSchedul_Create(
        &(EventSchedul_Allocator){malloc, free});
    TEST_ASSERT(ctx != NULL, "Create succeeded");

    /* start=0xfff0 > end=0x0010：合法区间为 [0xfff0, 0xffff] ∪ [0x0000, 0x000f] */
    EventSchedul_TaskNode cfg = { .pTaskFunc = test_task_cb, .pTaskFuncArg = NULL,
        .info = { .eventStart = 0xfff0, .eventEnd = 0x0010 } };
    EventSchedul_TaskNode* h = EventSchedul_TaskRegister(ctx, &cfg);
    TEST_ASSERT(h != NULL, "Task registered (events [0xfff0, 0xffff] U [0, 0x000f])");

    /* 合法：低端 */
    TEST_ASSERT(EventSchedul_setEventToTask(ctx, h, 0xfff5) == EVTSCHEDUL_OK,
        "setEvent 0xfff5 (wrap low side) returns OK");
    /* 合法：高端 */
    TEST_ASSERT(EventSchedul_setEventToTask(ctx, h, 0x0005) == EVTSCHEDUL_OK,
        "setEvent 0x0005 (wrap high side) returns OK");

    /* 非法：中间区间 */
    TEST_ASSERT(EventSchedul_setEventToTask(ctx, h, 0x0050) == EVTSCHEDUL_ERR_EVENT,
        "setEvent 0x0050 (mid gap) returns ERR_EVENT");

    EventSchedul_Destroy(ctx);
}

/* --- 9. 注销后指针失效（setEvent 应拒绝已注销的 handle） --- */
void test_use_after_unregister(void) {
    printf("--- test_use_after_unregister ---\n");

    EventSchedul_Context* ctx = EventSchedul_Create(
        &(EventSchedul_Allocator){malloc, free});
    TEST_ASSERT(ctx != NULL, "Create succeeded");

    EventSchedul_TaskNode cfg = { .pTaskFunc = test_task_cb, .pTaskFuncArg = NULL,
        .info = { .eventStart = 0x0001, .eventEnd = 0x0100 } };
    EventSchedul_TaskNode* h = EventSchedul_TaskRegister(ctx, &cfg);
    TEST_ASSERT(h != NULL, "Task registered");

    EventSchedul_TaskUnRegister(ctx, h);
    int ret = EventSchedul_setEventToTask(ctx, h, 0x0050);
    TEST_ASSERT(ret == EVTSCHEDUL_ERR_TASK,
        "setEvent with unregistered handle returns ERR_TASK");

    EventSchedul_Destroy(ctx);
}

/* ==================== 测试入口 ==================== */

int run_all_tests(void) {
    printf("\n========== EventScheduling API Test Suite ==========\n\n");

    test_create_destroy();
    test_task_register();
    test_register_full();
    test_task_unregister();
    test_set_event();
    test_tmos_poll();
    test_multi_instance();
    test_event_range_wrap();
    test_use_after_unregister();

    printf("\n====================================================\n");
    printf("Results: %d PASS, %d FAIL, %d TOTAL\n",
        g_pass, g_fail, g_pass + g_fail);

    return (g_fail == 0) ? 0 : 1;
}
