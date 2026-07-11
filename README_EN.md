# EventScheduling — A Simple Event Scheduler Using a Circular Task Queue

[中文](README.md)

## Features

- **Multi-instance support**: Opaque handle pattern — create multiple independent scheduler instances simultaneously
- **Dual-mode task management**: Static array mode (compile-time capacity) and dynamic allocation mode (runtime expansion)
- **Dependency injection**: Memory allocator injected via callbacks, core module free from `stdlib.h` dependency (only `memset`/`memcpy` retained)
- **Intrusive linked list**: Based on the Linux kernel-style `c-linked-list`, type-safe with zero extra allocation
- **TMOS friendly**: Non-blocking `TmosPoll` interface suitable for cooperative multitasking (no interrupt context saving)
- **Event range validation**: Tasks support contiguous ranges `[start, end)` or wrap-around ranges `start > end`

## Architecture

```
┌─────────────────────────────────────────────┐
│                  main.c                      │
│   ┌─────────┐   ┌────────────────────────┐  │
│   │ Memory  │──▶│  EventSchedul_Allocator │  │
│   │  Pool   │   │  {malloc, free}         │  │
│   └─────────┘   └───────────┬────────────┘  │
├──────────────────────────────┼──────────────┤
│                  EventSchedul               │
│   ┌──────────────┐  ┌───────┴───────────┐  │
│   │  taskPool[]  │  │   ringTaskQueue   │  │
│   │ (static arr) │  │  (ring buffer)    │  │
│   └──────────────┘  └───────────────────┘  │
│   ┌──────────────────────────────────────┐  │
│   │         c-linked-list                 │  │
│   │  (intrusive doubly-linked, no heap)  │  │
│   └──────────────────────────────────────┘  │
└─────────────────────────────────────────────┘
```

## Quick Start

```c
#include "EventSchedul.h"

/* Task callback */
static void myTask(EventSchedul_EventId evt, void* arg) {
    printf("Received event: 0x%04x\n", evt);
}

int main(void) {
    /* Create a scheduler instance with injected malloc/free */
    EventSchedul_Context* ctx = EventSchedul_Create(
        &(EventSchedul_Allocator){malloc, free});

    /* Register a task (listens for events in [0x0001, 0x0010)) */
    EventSchedul_TaskNode cfg = {
        .pTaskFunc = myTask,
        .pTaskFuncArg = NULL,
        .info = { .eventStart = 0x0001, .eventEnd = 0x0010 }
    };
    EventSchedul_TaskNode* handle = EventSchedul_TaskRegister(ctx, &cfg);

    /* Post an event */
    EventSchedul_setEventToTask(ctx, handle, 0x0005);

    /* TMOS non-blocking poll */
    EventSchedul_TmosPoll(ctx); // → myTask(0x0005, NULL)

    /* Cleanup */
    EventSchedul_TaskUnRegister(ctx, handle);
    EventSchedul_Destroy(ctx);
    return 0;
}
```

## API Reference

| Function | Description |
|---|---|
| `EventSchedul_Create(allocator)` | Create a scheduler instance with injected allocator |
| `EventSchedul_Destroy(ctx)` | Destroy instance, free all resources |
| `EventSchedul_TaskRegister(ctx, cfg)` | Register a task, returns a handle |
| `EventSchedul_TaskUnRegister(ctx, handle)` | Unregister a task |
| `EventSchedul_setEventToTask(ctx, handle, evt)` | Post an event to a task (enqueue) |
| `EventSchedul_TmosPoll(ctx)` | Non-blocking fetch and execute one event (TMOS-compatible) |
| `EventSchedul_MainLoop(ctx)` | Blocking main loop (requires `RegSleepMethod` first) |
| `EventSchedul_RegSleepMethod(ctx, fn)` | Register sleep callback for blocking main loop |

### Return Values

```c
typedef enum {
    EVTSCHEDUL_OK         = 0,   // Success
    EVTSCHEDUL_ERR_FAIL,         // General failure
    EVTSCHEDUL_ERR_ARG,          // Invalid argument
    EVTSCHEDUL_ERR_MEM,          // Out of memory
    EVTSCHEDUL_ERR_NOTHING,      // No data
    EVTSCHEDUL_ERR_EVENT,        // Event error (out of range)
    EVTSCHEDUL_ERR_TASK,         // Task error (not registered)
} EventSchedul_ErrCode;
```

### Type Definitions

| Type | Underlying | Description |
|---|---|---|
| `EventSchedul_EventId` | `unsigned short` | Event ID (0x0000–0xFFFF) |
| `EventSchedul_TaskId` | `short` | Task ID |
| `EventSchedul_ExecCount` | `unsigned short` | Execution count |
| `EventSchedul_Allocator` | `{malloc_fn, free_fn}` | Injected memory allocator |

## Configuration Macros

Macros are defined in `include/EventSchedul.h`:

| Macro | Default | Description |
|---|---|---|
| `EVTSCHEDUL_TASKS_MODE` | `EVTSCHEDUL_STATIC` | Task management mode: `EVTSCHEDUL_STATIC`(0) / `EVTSCHEDUL_DYNAMIC`(1) |
| `EVTSCHEDUL_TASKS_MAX` | `8` | Max tasks in static mode |
| `EVTSCHEDUL_TASKS_QUEUE_MAX` | `EVTSCHEDUL_TASKS_MAX * 2` | Ring event queue depth |
| `EVTSCHEDUL_INVALID_EVT` | `0xFFFF` | Invalid event ID |
| `EVTSCHEDUL_INVALID_TASK_ID` | `-1` | Invalid task ID |

## Building

```bash
mkdir build && cd build
cmake .. -G Ninja             # or -G "MinGW Makefiles"
cmake --build .
```

Output at `build/bin/EventScheduling.exe`.

## Running Tests

```bash
.\build\bin\EventScheduling.exe
```

Includes 9 test suites covering all public APIs (61 assertions).

## Third-Party Acknowledgments

| Library | Repository | Purpose |
|---|---|---|
| ringBuffer | [PING020903/ringBuffer](https://github.com/PING020903/ringBuffer) | Ring FIFO buffer for event queuing |
| MemoryGroundPlus | [PING020903/MemoryGroundPlus](https://github.com/PING020903/MemoryGroundPlus) | Memory pool, replaces malloc in dynamic mode |
| c-linked-list | [embeddedartistry/c-linked-list](https://github.com/embeddedartistry/c-linked-list) | Linux kernel-style intrusive doubly-linked list |

## Directory Structure

```
EventScheduling/
├── include/
│   ├── EventSchedul.h          # Public header
│   └── DBG_macro.h             # Debug macros
├── sources/
│   ├── EventSchedul.c          # Core scheduler implementation
│   ├── main.c                  # Sample entry point
│   └── test.c                  # API test suite
├── thirdparty/
│   ├── ringBuffer/             # Ring buffer
│   ├── memoryGroundPlus/       # Memory pool
│   └── c-linked-list/          # Intrusive linked list
├── CmakeLists.txt
└── README.md
```
