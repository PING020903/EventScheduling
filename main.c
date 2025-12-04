#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "Windows.h"
#include "FairSchedul.h"
#include "EventSchedul.h"
#include "Q8_8.h"
#include "DBG_macro.h"


#define test_func_addr(name) test_##name

#define create_test_func(name)                                     \
    static int name##_conut = 0;                                   \
    static void test_##name(void *arg)                             \
    {                                                              \
        FairSchedul_TaskNode** taskCfg = NULL;\
        if(arg!=NULL){\
            taskCfg = arg;\
            DEBUG_PRINT("count:[%d], [%u]", name##_conut, (*taskCfg)->status.numberID);} \
        else{\
            DEBUG_PRINT("count:[%d]", name##_conut);\
        }\
        name##_conut++;                                            \
        Sleep(500);                                               \
    }

create_test_func(task1)
create_test_func(task2)
create_test_func(task3)
create_test_func(task4)

static void MyShutdown(void* arg)
{
    //scheduler_shutdown();
    DEBUG_PRINT("");
    Sleep(1000);
}

static void myEvtTask(unsigned short recvEvt, void* arg) {
    DEBUG_PRINT("");
    if (arg == NULL)
        return;
    EventSchedul_TaskNode* taskHandle = *(EventSchedul_TaskNode**)arg;
    EventSchedul_setEventToTask(taskHandle, 0x0001);
    EventSchedul_setEventToTask(taskHandle, 0x0002);
    DEBUG_PRINT("set event 0x0001, trigger event:%u", recvEvt);
}

static void delay2s(void* arg) {
    Sleep(2000);
}


int main(void)
{
    system("cls");
    Sleep(2000);
    printf("this is cmake study project.\n");
    printf("study Ninja ! ! !\n");
    // 创建任务参数
    FairSchedul_TaskNode* task1_arg,
        * task2_arg,
        * task3_arg,
        * task4_arg;
    FairSchedul_TaskNode cfg = TASK_CFG(test_func_addr(task1), &task1_arg, 8);
    EventSchedul_TaskNode* handle = NULL;
    EventSchedul_TaskNode eCfg = { .pTaskFunc = myEvtTask,
        .pTaskFuncArg = &handle,
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
    

    test_q4_12();
    handle = EventSchedul_TaskRegister(&eCfg);
    EventSchedul_RegSleepMethod(delay2s);
#if EVTSCHEDUL_TEST
    evtSchedul_test(handle);
#endif
    DEBUG_PRINT("EVTSCHEDUL UNREG [%d]", EventSchedul_TaskUnRegister(handle));

    

#if 1
    // 创建任务节点
    task1_arg = create_task(&cfg, CYCLE_ONCE);
    // 由于主函数不会主动退出, 故此用局部变量作为任务参数

    cfg.functionCallback = test_func_addr(task2);
    cfg.functionArgment = &task2_arg;
    cfg.SetPriority = 7;
    task2_arg = create_task(&cfg, CYCLE_PERIOD);

    cfg.functionCallback = test_func_addr(task3);
    cfg.functionArgment = &task3_arg;
    cfg.SetPriority = 0;
    task3_arg = create_task(&cfg, CYCLE_PERIOD);

    cfg.functionCallback = test_func_addr(task4);
    cfg.functionArgment = &task4_arg;
    cfg.SetPriority = 4;
    task4_arg = create_task(&cfg, CYCLE_PERIOD);
#endif
    cfg.functionCallback = MyShutdown;
    cfg.functionArgment = NULL;
    cfg.SetPriority = 0;
    create_task(&cfg, CYCLE_PERIOD);

    // 运行调度器
    scheduler_run();

    system("pause");
    return 0;
}
