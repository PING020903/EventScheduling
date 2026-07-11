#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "EventSchedul.h"
#include "memGroundP.h"
#include "DBG_macro.h"

/* 从 test.c 引入测试入口 */
extern int run_all_tests(void);

char __DBG_string[DBG_DEFAULT_BUFFER_LEN];

/* ==================== MGP 内存池 ==================== */
/* 任务节点用内存池：8个节点 × ~48字节 + MGP开销 ≈ 需要 2KB */
#define EVT_POOL_SIZE 2048
static uint8_t g_mgp_mem[EVT_POOL_SIZE];
static mgp_t    g_mgp_pool = NULL;

/* MGP → EventSchedul 适配器（闭包，使 mgp_t pool 适配 malloc/free 签名） */
static void* mgp_adapt_malloc(size_t size)
{
    return mgp_malloc(g_mgp_pool, size);
}

static void mgp_adapt_free(void* ptr)
{
    mgp_free(g_mgp_pool, ptr);
}

static void MyShutdown(void* arg)
{
    //scheduler_shutdown();
    DEBUG_PRINT("");
    Sleep(1000);
}

/* 回调参数结构体：将 ctx 和 taskHandle 打包传给任务回调 */
typedef struct {
    EventSchedul_Context*  ctx;
    EventSchedul_TaskNode* handle;
} MyTaskArg;

static void myEvtTask(EventSchedul_EventId recvEvt, void* arg) {
    DEBUG_PRINT("");
    if (arg == NULL)
        return;
    MyTaskArg* taskArg = (MyTaskArg*)arg;
    EventSchedul_setEventToTask(taskArg->ctx, taskArg->handle, 0x0001);
    EventSchedul_setEventToTask(taskArg->ctx, taskArg->handle, 0x0002);
    DEBUG_PRINT("set event 0x0001, trigger event:%u", recvEvt);
}

static void delay2s(void* arg) {
    Sleep(2000);
}


int main(void)
{
    EventSchedul_Context* ctx = NULL;

#if (EVTSCHEDUL_TASKS_MODE == EVTSCHEDUL_DYNAMIC)
    // 注入排序/查找函数给 MGP（用户可替换为自定义实现）
    mgp_set_sort_func(qsort);
    mgp_set_bsearch_func(bsearch);

    // 创建 MGP 内存池
    g_mgp_pool = mgp_create_with_pool(g_mgp_mem, EVT_POOL_SIZE);

    // 用 MGP 适配器创建调度器实例（分配器在 Create 时注入）
    ctx = EventSchedul_Create(&(EventSchedul_Allocator){mgp_adapt_malloc, mgp_adapt_free});
#else
    // 静态模式：用标准库 malloc/free（仅用于上下文自身分配）
    ctx = EventSchedul_Create(&(EventSchedul_Allocator){malloc, free});
#endif
    if (!ctx) {
        printf("Failed to create scheduler context!\n");
        return -1;
    }

    system("cls");
    Sleep(2000);
    printf("this is cmake study project.\n");
    printf("study Ninja ! ! !\n");

    /* ===== 运行 API 测试套件 ===== */
    run_all_tests();

    /* 回调参数：将 ctx 和 handle 打包传递给任务 */
    MyTaskArg taskArg = { .ctx = ctx, .handle = NULL };
    EventSchedul_TaskNode eCfg = { .pTaskFunc = myEvtTask,
        .pTaskFuncArg = &taskArg,
#if 0
        .info.eventStart = 0x0000,
        .info.eventEnd = 0x000f
#endif
#if 0
        .info.eventStart = 0xfff1,
        .info.eventEnd = 0x000f
#endif
#if 1
        .info.eventStart = 0x0000,
        .info.eventEnd = 0xffff
#endif
    };
    

    taskArg.handle = EventSchedul_TaskRegister(ctx, &eCfg);
    EventSchedul_RegSleepMethod(ctx, delay2s);
#if EVTSCHEDUL_TEST
    evtSchedul_test(ctx, taskArg.handle);
#endif
    DEBUG_PRINT("EVTSCHEDUL UNREG [%d]", EventSchedul_TaskUnRegister(ctx, taskArg.handle));

    // 运行事件调度主循环
    EventSchedul_MainLoop(ctx);

    // 清理调度器
    EventSchedul_Destroy(ctx);

    system("pause");
    return 0;
}
