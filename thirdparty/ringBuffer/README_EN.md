# Ring Buffer

A lightweight C ring buffer implementation with **variable-size item support**, allowing direct storage of structs without serialization. Ideal for embedded systems. Current version **v1.2.0**.

## ✨ Core Features

### Variable Item Size (Key Feature)

Unlike traditional ring buffers that only support `uint8_t`, this library supports **arbitrary-sized data items** through the `item_size` parameter:

```c
// ✅ Store structs directly, no serialization needed
typedef struct { uint32_t ts; float temp; } SensorData;

static uint8_t buf[sizeof(SensorData) * 100];
static ringbuf_t rb = RINGBUFCRTL_INIT(buf, 100, sizeof(SensorData), false);

SensorData data = {...};
ringBuf_push(&rb, &data);  // Use directly!
```

**Comparison with other implementations:**
- ❌ Most C implementations: Single-byte only, requires manual serialization
- ✅ FreeRTOS Queue: Variable-size support, but requires dynamic memory
- ✅ **This library**: Variable-size + zero-copy + no dynamic memory + type-safe

## Other Features

- 🚀 **High-performance indexing**: Extended index range (0~2*depth-1), reduces modulo operations by ~50%
- 🔄 **Flexible modes**: Supports overwrite/non-overwrite modes
- 📦 **Batch operations**: Provides `push_multi` / `pop_multi` APIs
- 💾 **Zero-copy**: Uses user-provided static memory, no dynamic allocation
- 🔍 **Peek functionality**: View data at specific indices without removal
- 🧪 **Complete test suite**: Includes test cases for various usage scenarios
- 🛡️ **Type safety**: Dedicated type aliases (`ringbuf_uidx_t`, `ringbuf_cnt_t`, etc.) to prevent overflow in deep buffers
- ✅ **Parameter validation**: `RINGBUF_ARG_CHECK` macro validates non-zero `depth`/`item_size` and prevents `2*depth` overflow of index type, automatically checked on every API call
- 🔒 **Mutex interface reserved**: Header defines `ringbuf_mutex_t`, `ringbuf_lock_func_t`, `ringbuf_unlock_func_t` types for integrating external synchronization

## ⚠️ Important Notice

**This project is implemented for bare-metal environments only, without thread-safety protection.**

When using in RTOS or multi-threaded environments, external mutex locks or critical section protection must be added.

## Quick Start

```c
#include "ringBuffer.h"

// 1. Define data type and buffer
typedef struct { uint32_t ts; float value; } DataItem;
static uint8_t buf[sizeof(DataItem) * 50];
static ringbuf_t rb = RINGBUFCRTL_INIT(buf, 50, sizeof(DataItem), false);

int main(void) {
    ringBuf_init(&rb);
    
    // 2. Write data
    DataItem item = {.ts = 12345, .value = 25.5f};
    ringBuf_push(&rb, &item);
    
    // 3. Read data
    DataItem received;
    if (ringBuf_pop(&rb, &received) == RINGBUF_OK) {
        // Process data
    }
    
    return 0;
}
```

### Peek Functionality Example

```c
// View data at specific position without removing
DataItem peeked;
if (ringBuf_peek(&rb, &peeked, 0) == RINGBUF_OK) {
    // View the first element
}

// Batch view multiple elements
DataItem peekArray[3];
short peeked_count = 0;
ringBuf_peek_multi(&rb, peekArray, 3, 0, &peeked_count);
```

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

Or run `build.bat` on Windows.

## API Reference

| Function | Description |
|----------|-------------|
| `ringBuf_init(rb)` | Initialize the buffer |
| `ringBuf_clear(rb)` | Clear the buffer |
| `ringBuf_push(rb, &data)` | Push a single item |
| `ringBuf_pop(rb, &data)` | Pop and remove an item |
| `ringBuf_peek(rb, &data, index)` | Peek at data at specified index without removing |
| `ringBuf_push_multi(rb, data, count, &written)` | Batch push |
| `ringBuf_pop_multi(rb, data, count, &read)` | Batch pop |
| `ringBuf_peek_multi(rb, data, count, start_index, &peeked)` | Batch peek at data starting from specified index |
| `ringBuf_count(rb, &count)` | Get current item count (returned via pointer) |

**Return values:** Most functions return `RINGBUF_OK` on success, error codes otherwise (`RINGBUF_ERR_EMPTY`, `RINGBUF_ERR_WR_DENIED`, `RINGBUF_ERR_IDX`, etc.). `ringBuf_count` returns count via `ringbuf_cnt_t *pCount` pointer.

## Technical Highlights

1. **Extended index range**: Index range 0~2*depth-1, reduces modulo frequency by 50%
2. **Zero-copy design**: Directly uses user-provided static memory
3. **Type safety**: Dedicated type aliases (`ringbuf_uidx_t`/`ringbuf_ucnt_t`/`ringbuf_idx_t`/`ringbuf_cnt_t`) ensure consistent index and count variable types
4. **Parameter validation**: `RINGBUF_ARG_CHECK` macro centralizes validation across all API entry points, preventing zero `depth`/`item_size` or `2*depth` overflow of index type
5. **Overwrite strategy**: Optional overwrite mode (discard old data) or non-overwrite mode (return error)
6. **Peek functionality**: View data at specific indices without affecting read/write pointers
7. **Batch operations**: Efficient batch data processing capability
8. **Mutex interface**: `ringBuf_lock_func_t` / `ringBuf_unlock_func_t` function pointer types for RTOS synchronization integration

See [ringBuffer.h](include/ringBuffer.h) for complete API details.

## Notes

1. The ring buffer structure must be properly initialized before use
2. Ensure data pointers are valid and have sufficient space
3. Pay special attention to reentrancy when used in interrupt service routines
4. Add appropriate synchronization mechanisms for multi-threaded environments
5. Peek operations do not change the read/write pointer state
6. Batch operations will process as much data as possible, returning the count of successfully processed items even if some fail

## License

This project is for learning purposes and can be freely used and modified as needed.
