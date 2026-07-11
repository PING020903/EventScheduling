# 环形缓冲区 (Ring Buffer)

支持**不定长数据项**的轻量级C语言环形缓冲区实现，可直接存储结构体而无需序列化，适合嵌入式系统。当前版本 **v1.2.0**。

## ✨ 核心特性

### 不定长 Item Size（主要特点）

与传统仅支持 `uint8_t` 的环形缓冲区不同，本库通过 `item_size` 参数支持**任意大小的数据项**：

```c
// ✅ 直接存储结构体，无需序列化
typedef struct { uint32_t ts; float temp; } SensorData;

static uint8_t buf[sizeof(SensorData) * 100];
static ringbuf_t rb = RINGBUFCRTL_INIT(buf, 100, sizeof(SensorData), false);

SensorData data = {...};
ringBuf_push(&rb, &data);  // 直接使用！
```

**对比其他实现：**
- ❌ 大多数C实现：仅支持单字节，需手动序列化
- ✅ FreeRTOS Queue：支持不定长，但需动态内存
- ✅ **本库**：不定长 + 零拷贝 + 无动态内存 + 类型安全

## 其他特性

- 🚀 **高性能索引**：扩展索引范围(0~2*depth-1)，减少取模运算约50%
- 🔄 **灵活模式**：支持覆盖/非覆盖模式
- 📦 **批量操作**：提供 `push_multi` / `pop_multi` API
- 💾 **零拷贝**：使用用户提供的静态内存，无动态分配
- 🔍 **Peek 功能**：支持按索引查看数据而不移除
- 🧪 **完整测试套件**：包含多种使用场景的测试用例
- 🛡️ **类型安全**：使用专用类型别名（`ringbuf_uidx_t`、`ringbuf_cnt_t` 等），避免大深度缓冲区下的溢出风险
- ✅ **参数校验**：`RINGBUF_ARG_CHECK` 宏统一校验 `depth`/`item_size` 非零及 `2*depth` 不溢出索引类型，所有 API 调用自动检查
- 🔒 **预留互斥锁接口**：头文件中定义了 `ringbuf_mutex_t`、`ringbuf_lock_func_t`、`ringbuf_unlock_func_t` 类型，方便集成外部同步机制

## ⚠️ 重要提示

**本项目仅针对裸机环境实现，未实现线程安全保护。**

在RTOS或多线程环境中使用时，需外部添加互斥锁或临界区保护。

## 快速开始

```c
#include "ringBuffer.h"

// 1. 定义数据类型和缓冲区
typedef struct { uint32_t ts; float value; } DataItem;
static uint8_t buf[sizeof(DataItem) * 50];
static ringbuf_t rb = RINGBUFCRTL_INIT(buf, 50, sizeof(DataItem), false);

int main(void) {
    ringBuf_init(&rb);
    
    // 2. 写入数据
    DataItem item = {.ts = 12345, .value = 25.5f};
    ringBuf_push(&rb, &item);
    
    // 3. 读取数据
    DataItem received;
    if (ringBuf_pop(&rb, &received) == RINGBUF_OK) {
        // 处理数据
    }
    
    return 0;
}
```

### Peek 功能示例

```c
// 查看指定位置的数据而不移除
DataItem peeked;
if (ringBuf_peek(&rb, &peeked, 0) == RINGBUF_OK) {
    // 查看第一个元素
}

// 批量查看多个元素
DataItem peekArray[3];
short peeked_count = 0;
ringBuf_peek_multi(&rb, peekArray, 3, 0, &peeked_count);
```

## 构建

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

或在Windows上运行 `build.bat`

## API 参考

| 函数 | 说明 |
|------|------|
| `ringBuf_init(rb)` | 初始化缓冲区 |
| `ringBuf_clear(rb)` | 清空缓冲区 |
| `ringBuf_push(rb, &data)` | 写入单个数据项 |
| `ringBuf_pop(rb, &data)` | 读取并移除 |
| `ringBuf_peek(rb, &data, index)` | 查看指定索引位置的数据但不移除 |
| `ringBuf_push_multi(rb, data, count, &written)` | 批量写入 |
| `ringBuf_pop_multi(rb, data, count, &read)` | 批量读取 |
| `ringBuf_peek_multi(rb, data, count, start_index, &peeked)` | 批量查看指定起始位置的数据 |
| `ringBuf_count(rb, &count)` | 获取当前数据项数量（通过指针返回） |

**返回值：** 多数函数返回 `RINGBUF_OK` 表示成功，其他为错误码（`RINGBUF_ERR_EMPTY`, `RINGBUF_ERR_WR_DENIED`, `RINGBUF_ERR_IDX` 等）。`ringBuf_count` 通过 `ringbuf_cnt_t *pCount` 指针返回计数值。

## 技术亮点

1. **扩展索引范围**：索引范围 0~2*depth-1，取模频率降低50%
2. **零拷贝设计**：直接使用用户提供的静态内存
3. **类型安全**：专用类型别名（`ringbuf_uidx_t`/`ringbuf_ucnt_t`/`ringbuf_idx_t`/`ringbuf_cnt_t`）确保索引和计数变量类型一致
4. **参数校验**：`RINGBUF_ARG_CHECK` 宏集中校验，覆盖所有 API 入口，防止 `depth`/`item_size` 为零或 `2*depth` 溢出索引类型
5. **覆盖策略**：可选覆盖模式（丢弃旧数据）或非覆盖模式（返回错误）
6. **Peek 功能**：支持按索引查看数据而不影响读写指针
7. **批量操作**：高效的批量数据处理能力
8. **互斥锁预留**：`ringBuf_lock_func_t` / `ringBuf_unlock_func_t` 函数指针类型，方便集成 RTOS 同步机制

完整API详见 [ringBuffer.h](include/ringBuffer.h)

## 注意事项

1. 使用前必须正确初始化环形缓冲区结构
2. 确保传入的数据指针有效且具有足够空间
3. 在中断服务程序中使用需特别注意重入问题
4. 如需在多线程环境中使用，请添加适当的同步机制
5. Peek 操作不会改变读写指针状态
6. 批量操作会尽可能多地处理数据，即使部分失败也会返回已处理的数量

## 许可证

本项目为学习用途，可根据需要自由使用和修改。