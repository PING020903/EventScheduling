#ifndef _EVENTSCHEDUL_H_
#define _EVENTSCHEDUL_H_

#define EVTSCHEDUL_DYNAMIC 0
#define EVTSCHEDUL_STATIC 1
#define EVTSCHEDUL_TASKS_MODE EVTSCHEDUL_DYNAMIC

#define EVTSCHEDUL_TEST 0

#define EVTSCHEDUL_INVALID_EVT 0xffffU
#define EVTSCHEDUL_INIT_EVT 0x0000U

#define EVTSCHEDUL_INIT_TASK_ID 0U
#define EVTSCHEDUL_INVALID_TASK_ID (short)-1

#define EVTSCHEDUL_TASKS_MAX 8
#define EVTSCHEDUL_TASKS_QUEUE_MAX (EVTSCHEDUL_TASKS_MAX * 2)

#define EVTSCHEDUL_OK 0
#define EVTSCHEDUL_ERR_FAIL -1
#define EVTSCHEDUL_ERR_ARG -2
#define EVTSCHEDUL_ERR_MEM -3
#define EVTSCHEDUL_ERR_NOTHING -4
#define EVTSCHEDUL_ERR_EVENT -5
#define EVTSCHEDUL_ERR_TASK -6

/*
* 若短时间内有不同的事件被触发,
* 可能会导致最后触发的事件把先前的事件都覆盖
* 
*/



typedef struct {
    unsigned short eventStart; // 事件起始号
    unsigned short eventEnd; // 事件结束号
    unsigned short eventTrigger; // 触发事件号
    short taskId; // 任务编号(由注册任务函数决定)
    unsigned short executeCount; // 任务运行次数
}EventSchedul_TaskNodeInfo;

typedef void (*EventSchedul_pTaskFunc)(const unsigned short RecvEvt, void* arg);
typedef struct EventSchedul_TaskNode {
    EventSchedul_TaskNodeInfo info;
    EventSchedul_pTaskFunc pTaskFunc;
    void* pTaskFuncArg;
    struct EventSchedul_TaskNode* next;
    struct EventSchedul_TaskNode* prev;
}EventSchedul_TaskNode;

typedef struct {
    EventSchedul_TaskNode* taskHandle;
    unsigned short eventTrigger;
}EventSchedul_TaskQueue;

EventSchedul_TaskNode* EventSchedul_TaskRegister(const EventSchedul_TaskNode* cfg);

int EventSchedul_TaskUnRegister(EventSchedul_TaskNode* taskHandle);

int EventSchedul_setEventToTask(const EventSchedul_TaskNode* task,
    const unsigned short TaskEvent);

#if EVTSCHEDUL_TEST
void evtSchedul_test(const EventSchedul_TaskNode* task);
#endif

int EventSchedul_RegSleepMethod(void* pFunc);

int EventSchedul_MainLoop(void);

#endif // !_EVENTSCHEDUL_H_
