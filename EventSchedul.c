#include "DBG_macro.h"
#include "stdlib.h"
#include "EventSchedul.h"

#define EVTSCHEDUL_DEBUG_CREATE_TASK 1

#if (EVTSCHEDUL_TASKS_MODE == EVTSCHEDUL_STATIC)
static EventSchedul_TaskNode taskList_Static[EVTSCHEDUL_TASKS_MAX] = { 0 };
#endif

#if (EVTSCHEDUL_TASKS_MODE == EVTSCHEDUL_DYNAMIC)
static EventSchedul_TaskNode* taskList_Dynamic = NULL;
#endif

static void (*sleepMethod)(void);

// 记录创建过的任务数量
static short taskNum = EVTSCHEDUL_INIT_TASK_ID;

// 事件环形队列(tips: 环形队列一般只有队列长度-1是真正被使用的)
static EventSchedul_TaskQueue RingTaskQueue[EVTSCHEDUL_TASKS_QUEUE_MAX] = { 0 };
static unsigned char ringTaskQueue_head = 0,
ringTaskQueue_tail = 0;

// 将任务事件推入队列
static inline int pushTaskToQueue(const EventSchedul_TaskQueue* task) {
    unsigned char nextHead = (ringTaskQueue_head + 1) % EVTSCHEDUL_TASKS_QUEUE_MAX;
    if (!task)
        return EVTSCHEDUL_ERR_ARG;
    if (nextHead == ringTaskQueue_tail)
        return EVTSCHEDUL_ERR_MEM;

    // 推入任务句柄&触发事件值
    RingTaskQueue[ringTaskQueue_head].taskHandle = task->taskHandle;
    RingTaskQueue[ringTaskQueue_head].eventTrigger = task->eventTrigger;

    ringTaskQueue_head = nextHead;
    return EVTSCHEDUL_OK;
}

// 从队列中取出任务事件
static inline int pullTaskWithQueue(EventSchedul_TaskNode** task) {
    if (!task && (*task) != NULL)
        return EVTSCHEDUL_ERR_ARG;

    if (ringTaskQueue_head == ringTaskQueue_tail)
        return EVTSCHEDUL_ERR_NOTHING;

    // 取出任务句柄&触发事件值
    *task = RingTaskQueue[ringTaskQueue_tail].taskHandle;
    (*task)->info.eventTrigger = RingTaskQueue[ringTaskQueue_tail].eventTrigger;

    ringTaskQueue_tail = (ringTaskQueue_tail + 1) % EVTSCHEDUL_TASKS_QUEUE_MAX;
    return EVTSCHEDUL_OK;
}

// 寻找或创建空闲节点
static EventSchedul_TaskNode* FindOrCreateFreeTaskNode(void) {
    EventSchedul_TaskNode* task = NULL, * current = NULL;
#if (EVTSCHEDUL_TASKS_MODE == EVTSCHEDUL_STATIC)
    int pos1 = 0, pos2 = 0;
    // 为静态表的节点初始化上下节点地址
    for (int i = 0; i < SIZE_ARRARY(taskList_Static); i++) {
        taskList_Static[i].next = &taskList_Static[i + 1];
        pos1 = SIZE_ARRARY(taskList_Static) - i - 1;
        pos2 = pos1 - 1;
        if (pos2 > -1) {
            taskList_Static[pos1].prev = &taskList_Static[pos2];
        }
    }
    taskList_Static[0].prev = &taskList_Static[SIZE_ARRARY(taskList_Static) - 1];
    taskList_Static[SIZE_ARRARY(taskList_Static) - 1].next = &taskList_Static[0];

    current = &taskList_Static[0];
#endif
#if (EVTSCHEDUL_TASKS_MODE == EVTSCHEDUL_DYNAMIC)
    current = taskList_Dynamic;
    if (current == NULL)
        goto _NewNode;
#endif
    do {
#if EVTSCHEDUL_DEBUG_CREATE_TASK
        DEBUG_PRINT("current[%p], current.next[%p], current.prev[%p]",
            current, current->next, current->prev);
#endif
        // 任务钩子为空, 且任务ID无效或处于初始化状态
        if (current->pTaskFunc == NULL &&
            (current->info.taskId == EVTSCHEDUL_INIT_TASK_ID ||
                current->info.taskId == EVTSCHEDUL_INVALID_TASK_ID)) {
            return current;
        }
        current = current->next;
#if (EVTSCHEDUL_TASKS_MODE == EVTSCHEDUL_STATIC)
    } while (current != &taskList_Static[0] && current != NULL);
    MACRO_PRINT_INT(EVTSCHEDUL_ERR_MEM);
    return NULL;
#endif
#if (EVTSCHEDUL_TASKS_MODE == EVTSCHEDUL_DYNAMIC)
    } while (current != taskList_Dynamic && current != NULL);

_NewNode:
current = malloc(sizeof(EventSchedul_TaskNode));
if (current == NULL)
return NULL;

// 为新节点赋初值
memset(current, 0, sizeof(EventSchedul_TaskNode));

// 若头节点尚未创建
if (taskList_Dynamic == NULL) {
    taskList_Dynamic = current;
    current->next = current;
    current->prev = current;
    return current;
}

// 更新节点链
current->prev = taskList_Dynamic->prev;
current->next = taskList_Dynamic;
(taskList_Dynamic->prev)->next = current; // 为末端节点插入新节点
taskList_Dynamic->prev = current; // 为头节点接入新节点

return current;
#endif

// 给定固定返回值
MACRO_PRINT_INT(EVTSCHEDUL_ERR_FAIL);
return NULL;
}

// 注册任务
// 成功返回有效句柄地址, 失败返回NULL
EventSchedul_TaskNode* EventSchedul_TaskRegister(const EventSchedul_TaskNode* cfg) {
    EventSchedul_TaskNode* task = NULL;
    if (!cfg || !cfg->pTaskFunc)
        return NULL;

    task = FindOrCreateFreeTaskNode();
#if !EVTSCHEDUL_DEBUG_CREATE_TASK
    VAR_PRINT_POS(task);
#endif
    if (!task)
        return NULL;

    task->pTaskFunc = cfg->pTaskFunc;
    task->pTaskFuncArg = cfg->pTaskFuncArg;
    task->info.eventStart = cfg->info.eventStart;
    task->info.eventEnd = cfg->info.eventEnd;
    task->info.taskId = ++taskNum;

    // 限制任务ID为正整数
    if (taskNum < (short)0) {
#if (EVTSCHEDUL_TASKS_MODE == EVTSCHEDUL_DYNAMIC)
        free(task);
#endif
        return NULL;
    }
        


    VAR_PRINT_UD(task->info.taskId);
    return task;
}

int EventSchedul_TaskUnRegister(EventSchedul_TaskNode* taskHandle) {
    if (!taskHandle)
        return EVTSCHEDUL_ERR_ARG;

#if (EVTSCHEDUL_TASKS_MODE == EVTSCHEDUL_STATIC)
    // 清除任务信息
    memset(&(taskHandle->info), 0, sizeof(EventSchedul_TaskNodeInfo));

    // 清除任务钩子
    taskHandle->pTaskFunc = NULL;
    taskHandle->pTaskFuncArg = NULL;

    DEBUG_PRINT("1[%x] 2[%x] 3[%x] 4[%u] 5[%u]",
        taskHandle->info.eventStart,
        taskHandle->info.eventEnd,
        taskHandle->info.eventTrigger,
        taskHandle->info.taskId,
        taskHandle->info.executeCount);
#endif
#if (EVTSCHEDUL_TASKS_MODE == EVTSCHEDUL_DYNAMIC)
    // 更新节点链的链接逻辑
    (taskHandle->prev)->next = taskHandle->next;
    (taskHandle->next)->prev = taskHandle->prev;
    memset(taskHandle, 0, sizeof(EventSchedul_TaskNode));
    free(taskHandle);
#endif

    return EVTSCHEDUL_OK;
}

// 给任务发送事件
int EventSchedul_setEventToTask(const EventSchedul_TaskNode* task,
    const unsigned short TaskEvent) {
    int ret = EVTSCHEDUL_OK;
    EventSchedul_TaskNode* current = NULL;
    const EventSchedul_TaskQueue tmp = { .taskHandle = task, .eventTrigger = TaskEvent };
    if (!task)
        return EVTSCHEDUL_ERR_ARG;

#if (EVTSCHEDUL_TASKS_MODE == EVTSCHEDUL_STATIC)
    current = &taskList_Static[0];
#endif
#if (EVTSCHEDUL_TASKS_MODE == EVTSCHEDUL_DYNAMIC)
    current = taskList_Dynamic;
    if (current == NULL)
        return EVTSCHEDUL_ERR_FAIL;
#endif
    do {
        if (current == task) {
            goto _next;
        }

        current = current->next;
#if (EVTSCHEDUL_TASKS_MODE == EVTSCHEDUL_STATIC)
    } while (current != &taskList_Static[0] && current != NULL);
#endif
#if (EVTSCHEDUL_TASKS_MODE == EVTSCHEDUL_DYNAMIC)
    } while (current != taskList_Dynamic && current != NULL);
#endif
return EVTSCHEDUL_ERR_TASK; // 任务句柄中不存在此任务

_next:
switch (TaskEvent) {
case EVTSCHEDUL_INVALID_EVT:
case EVTSCHEDUL_INIT_EVT: return EVTSCHEDUL_ERR_ARG;
default:
    // 判断设置的事件值是否在注册的事件值区间
    ret = (task->info.eventStart > task->info.eventEnd) ? 1 : 0;
    if (ret) {
        // 中间区间没有注册
        if (TaskEvent < task->info.eventStart && TaskEvent >= task->info.eventEnd)
            return EVTSCHEDUL_ERR_EVENT;
    }
    else {
        // 左右区间没有注册
        if (TaskEvent < task->info.eventStart || TaskEvent >= task->info.eventEnd)
            return EVTSCHEDUL_ERR_EVENT;
    }

    break;
}

ret = pushTaskToQueue(&tmp);
DEBUG_PRINT("pushTaskToQueue:[%s]", (ret == EVTSCHEDUL_OK) ? "OK" : "FAIL");
return ret;
}

#if EVTSCHEDUL_TEST
static void test1(const unsigned short taskEvt, void* arg) {
    DEBUG_PRINT("");
}
static void test2(const unsigned short taskEvt, void* arg) {
    DEBUG_PRINT("");
}
static void test3(const unsigned short taskEvt, void* arg) {
    DEBUG_PRINT("");
}

void evtSchedul_test(const EventSchedul_TaskNode* task) {
    int ret = EVTSCHEDUL_OK;
    EventSchedul_TaskQueue testQueue = { .taskHandle = task };
    EventSchedul_TaskNode eCfg = { .pTaskFuncArg = NULL,
    .info.eventStart = 0x0001,
    .info.eventEnd = 0x000f
    };
    EventSchedul_TaskNode* current = NULL;
    if (!task)
        return;
#if 0
    testQueue.eventTrigger = 0x0001;
    ret = EventSchedul_setEventToTask(testQueue.taskHandle, testQueue.eventTrigger);
    DEBUG_PRINT("evt[%x], ret[%d]", testQueue.eventTrigger, ret);

    testQueue.eventTrigger = 0x0000;
    ret = EventSchedul_setEventToTask(testQueue.taskHandle, testQueue.eventTrigger);
    DEBUG_PRINT("evt[%x], ret[%d]", testQueue.eventTrigger, ret);

    testQueue.eventTrigger = 0xffff;
    ret = EventSchedul_setEventToTask(testQueue.taskHandle, testQueue.eventTrigger);
    DEBUG_PRINT("evt[%x], ret[%d]", testQueue.eventTrigger, ret);

    testQueue.eventTrigger = 0x000f;
    ret = EventSchedul_setEventToTask(testQueue.taskHandle, testQueue.eventTrigger);
    DEBUG_PRINT("evt[%x], ret[%d]", testQueue.eventTrigger, ret);

    testQueue.eventTrigger = 0x011f;
    ret = EventSchedul_setEventToTask(testQueue.taskHandle, testQueue.eventTrigger);
    DEBUG_PRINT("evt[%x], ret[%d]", testQueue.eventTrigger, ret);

    testQueue.eventTrigger = 0xfff1;
    ret = EventSchedul_setEventToTask(testQueue.taskHandle, testQueue.eventTrigger);
    DEBUG_PRINT("evt[%x], ret[%d]", testQueue.eventTrigger, ret);

    testQueue.eventTrigger = 0xfff2;
    ret = EventSchedul_setEventToTask(testQueue.taskHandle, testQueue.eventTrigger);
    DEBUG_PRINT("evt[%x], ret[%d]", testQueue.eventTrigger, ret);


    while (pullTaskWithQueue(&current) == EVTSCHEDUL_OK) {
        DEBUG_PRINT("--evt[%x]", current->info.eventTrigger);
    }
#endif // 简单测试环形FIFO队列, pass (2025.12.04)
#if 0
    testQueue.eventTrigger = 0x0001;
    VAR_PRINT_POS(taskList_Dynamic);
    ret = EventSchedul_setEventToTask(testQueue.taskHandle, testQueue.eventTrigger);

    EventSchedul_MainLoop();
#endif // 简单测试事件调度循环(静态条件), pass (2025.12.05)
       // 简单测试事件调度循环(动态条件), pass (2025.12.05)
#if 0
    eCfg.pTaskFunc = test1;
    current = EventSchedul_TaskRegister(&eCfg);
    VAR_PRINT_POS(current);

    eCfg.pTaskFunc = test2;
    testQueue.taskHandle = EventSchedul_TaskRegister(&eCfg);
    VAR_PRINT_POS(current);

    eCfg.pTaskFunc = test3;
    current = EventSchedul_TaskRegister(&eCfg);
    VAR_PRINT_POS(current);

    current = taskList_Dynamic;
    do {
        DEBUG_PRINT("current[%p], current.next[%p], current.prev[%p], taskId[%d]",
            current, current->next, current->prev, current->info.taskId);
        current = current->next;
    } while (current != taskList_Dynamic && current != NULL);

    DEBUG_PRINT("unreg ret[%d]", EventSchedul_TaskUnRegister(testQueue.taskHandle));
    current = taskList_Dynamic;
    do {
        DEBUG_PRINT("current[%p], current.next[%p], current.prev[%p], taskId[%d]",
            current, current->next, current->prev, current->info.taskId);
        current = current->next;
    } while (current != taskList_Dynamic && current != NULL);
#endif // 动态链表增加删除测试, pass (2025.12.05)
}
#endif

int EventSchedul_RegSleepMethod(void* pFunc) {
    if (!pFunc)
        return EVTSCHEDUL_ERR_ARG;

    sleepMethod = pFunc;
    return EVTSCHEDUL_OK;
}

int EventSchedul_MainLoop(void) {
    int ret = EVTSCHEDUL_OK;
    EventSchedul_TaskNode* current = NULL;
    if (!sleepMethod)
        goto _end;
    while (1) {
        current = NULL;
        ret = pullTaskWithQueue(&current);
        DEBUG_PRINT("pullTaskWithQueue:[%s]", (ret == EVTSCHEDUL_OK) ? "OK" : "FAIL");
        sleepMethod();
        if (ret != EVTSCHEDUL_OK || current == NULL)
            continue;

        current->pTaskFunc(current->info.eventTrigger, current->pTaskFuncArg);
        sleepMethod();
    }

_end:
    return EVTSCHEDUL_ERR_FAIL;
}
