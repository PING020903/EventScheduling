# EventScheduling — 基于环形任务队列的简单事件调度器

A Simple Event Scheduler Using a Circular Task Queue

[English](README_EN.md)

## 特性

- **多实例支持**：不透明句柄模式，可同时创建多个独立的调度器实例
- **双模式任务管理**：静态数组模式（编译期确定容量）和动态分配模式（运行时弹性扩展）
- **依赖注入**：内存分配器通过回调注入，核心模块不依赖 `stdlib.h`（仅保留 `memset`/`memcpy`）
- **入侵式链表**：基于 Linux 内核风格的 `c-linked-list`，类型安全、零额外分配
- **TMOS 友好**：非阻塞 `TmosPoll` 接口适用于协作式多任务调度（无中断保存上下文）
- **事件区间校验**：任务支持连续区间 `[start, end)` 或回绕区间 `start > end`

## 架构

```
┌─────────────────────────────────────────────┐
│                  main.c                     │
│   ┌─────────┐   ┌────────────────────────┐  │
│   │  MGP    │──▶│ EventSchedul_Allocator│  │
│   │ 内存池  │   │  {malloc, free}        │  │
│   └─────────┘   └───────────┬────────────┘  │
├──────────────────────────────┼──────────────┤
│                  EventSchedul               │
│   ┌──────────────┐  ┌───────┴───────────┐   │
│   │  taskPool[]  │  │   ringTaskQueue   │   │
│   │ (静态数组)   │  │   (环形缓冲区)     │   │
│   └──────────────┘  └───────────────────┘   │
│   ┌──────────────────────────────────────┐  │
│   │         c-linked-list                │  │
│   │     (入侵式双向链表，无堆分配)        │  │
│   └──────────────────────────────────────┘  │
└─────────────────────────────────────────────┘
```

## 快速开始

```c
#include "EventSchedul.h"

/* 任务回调 */
static void myTask(EventSchedul_EventId evt, void* arg) {
    printf("Received event: 0x%04x\n", evt);
}

int main(void) {
    /* 创建调度器实例，注入 malloc/free */
    EventSchedul_Context* ctx = EventSchedul_Create(
        &(EventSchedul_Allocator){malloc, free});

    /* 注册任务（监听事件区间 [0x0001, 0x0010)） */
    EventSchedul_TaskNode cfg = {
        .pTaskFunc = myTask,
        .pTaskFuncArg = NULL,
        .info = { .eventStart = 0x0001, .eventEnd = 0x0010 }
    };
    EventSchedul_TaskNode* handle = EventSchedul_TaskRegister(ctx, &cfg);

    /* 投递事件 */
    EventSchedul_setEventToTask(ctx, handle, 0x0005);

    /* TMOS 非阻塞轮询 */
    EventSchedul_TmosPoll(ctx); // → myTask(0x0005, NULL)

    /* 清理 */
    EventSchedul_TaskUnRegister(ctx, handle);
    EventSchedul_Destroy(ctx);
    return 0;
}
```

## API 参考

| 函数 | 说明 |
|---|---|
| `EventSchedul_Create(allocator)` | 创建调度器实例，注入内存分配器 |
| `EventSchedul_Destroy(ctx)` | 销毁实例，释放所有资源 |
| `EventSchedul_TaskRegister(ctx, cfg)` | 注册任务，返回句柄 |
| `EventSchedul_TaskUnRegister(ctx, handle)` | 注销任务 |
| `EventSchedul_setEventToTask(ctx, handle, evt)` | 向任务投递事件（入队） |
| `EventSchedul_TmosPoll(ctx)` | 非阻塞取一个事件并执行（TMOS 适用） |
| `EventSchedul_MainLoop(ctx)` | 阻塞式主循环（需先 `RegSleepMethod`） |
| `EventSchedul_RegSleepMethod(ctx, fn)` | 注册阻塞主循环的休眠回调 |

### 返回值

```c
typedef enum {
    EVTSCHEDUL_OK         = 0,   // 成功
    EVTSCHEDUL_ERR_FAIL,         // 通用失败
    EVTSCHEDUL_ERR_ARG,          // 参数错误
    EVTSCHEDUL_ERR_MEM,          // 内存不足
    EVTSCHEDUL_ERR_NOTHING,      // 无数据
    EVTSCHEDUL_ERR_EVENT,        // 事件错误（不在区间内）
    EVTSCHEDUL_ERR_TASK,         // 任务错误（未注册）
} EventSchedul_ErrCode;
```

### 类型定义

| 类型 | 底层 | 说明 |
|---|---|---|
| `EventSchedul_EventId` | `unsigned short` | 事件编号 (0x0000–0xFFFF) |
| `EventSchedul_TaskId` | `short` | 任务编号 |
| `EventSchedul_ExecCount` | `unsigned short` | 执行次数计数器 |
| `EventSchedul_Allocator` | `{malloc_fn, free_fn}` | 注入的内存分配器 |

## 配置宏

宏定义位于 `include/EventSchedul.h`：

| 宏 | 默认值 | 说明 |
|---|---|---|
| `EVTSCHEDUL_TASKS_MODE` | `EVTSCHEDUL_STATIC` | 任务管理模式：`EVTSCHEDUL_STATIC`(0) / `EVTSCHEDUL_DYNAMIC`(1) |
| `EVTSCHEDUL_TASKS_MAX` | `8` | 静态模式最大任务数 |
| `EVTSCHEDUL_TASKS_QUEUE_MAX` | `EVTSCHEDUL_TASKS_MAX * 2` | 环形事件队列深度 |
| `EVTSCHEDUL_INVALID_EVT` | `0xFFFF` | 无效事件号 |
| `EVTSCHEDUL_INVALID_TASK_ID` | `-1` | 无效任务编号 |

## 编译

```bash
mkdir build && cd build
cmake .. -G Ninja          # 或用 -G "MinGW Makefiles"
cmake --build .
```

产物在 `build/bin/EventScheduling.exe`。

## 运行测试

```bash
.\build\bin\EventScheduling.exe
```

内置 9 组测试用例，覆盖所有公共 API（61 个断言）。

## 第三方库致谢

| 库 | 仓库 | 用途 |
|---|---|---|
| ringBuffer | [PING020903/ringBuffer](https://github.com/PING020903/ringBuffer) | 环形 FIFO 缓冲区，用于事件队列 |
| MemoryGroundPlus | [PING020903/MemoryGroundPlus](https://github.com/PING020903/MemoryGroundPlus) | 内存池，动态模式下替代 malloc |
| c-linked-list | [embeddedartistry/c-linked-list](https://github.com/embeddedartistry/c-linked-list) | Linux 内核风格入侵式双向链表 |

## 目录结构

```
EventScheduling/
├── include/
│   ├── EventSchedul.h          # 公共头文件
│   └── DBG_macro.h             # 调试宏
├── sources/
│   ├── EventSchedul.c          # 调度器核心实现
│   ├── main.c                  # 示例入口
│   └── test.c                  # API 测试套件
├── thirdparty/
│   ├── ringBuffer/             # 环形缓冲区
│   ├── memoryGroundPlus/       # 内存池
│   └── c-linked-list/          # 入侵式链表
├── CmakeLists.txt
└── README.md
