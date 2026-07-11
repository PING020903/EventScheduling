# 更新日志 (CHANGELOG)

本文档记录项目的所有重要变更。

---

## [v1.2.0] - 2026-06-26

### ✨ 新增

- **参数校验增强**：`RINGBUF_ARG_CHECK` 宏新增 `depth`/`item_size` 非零校验，以及 `2*depth` 不溢出 `ringbuf_uidx_t` 的检查，统一应用于所有 API 入口（`init`/`push`/`pop`/`peek`/`count`/`clear`）
- **测试用例**：新增 Test 6 参数校验测试，覆盖 `depth=0`、`item_size=0`、超大缓冲区等边界场景

### 🔄 变更

- `ringBuf_init()` 移除重复参数校验，统一由 `RINGBUF_ARG_CHECK` 宏处理

---

## [v1.1.0] - 2026-06-26

### 🛠 修复

- **类型溢出问题**：引入类型别名 `ringbuf_uidx_t`、`ringbuf_ucnt_t`、`ringbuf_idx_t`、`ringbuf_cnt_t` 替代原始 `int`/`short`/`unsigned short`，解决索引和计数变量在大深度缓冲区场景下的潜在溢出风险

### 🔄 变更

- `ringbuf_t` 结构体中 `wr_idx`、`rd_idx` 改为 `ringbuf_uidx_t`（`unsigned int`）
- `ringbuf_t` 结构体中 `depth`、`item_size` 改为 `ringbuf_ucnt_t`（`unsigned short`）
- `ringBuf_peek()` 参数 `itemIdx` 类型由 `short` 改为 `ringbuf_ucnt_t`
- `ringBuf_push_multi()` / `ringBuf_pop_multi()` 的 `dataCount` 参数改为 `ringbuf_ucnt_t`，`pCount` 改为 `ringbuf_cnt_t *`
- `ringBuf_peek_multi()` 的 `dataCount` 改为 `ringbuf_ucnt_t`，`itemIdx` 改为 `ringbuf_cnt_t`，`pCount` 改为 `ringbuf_cnt_t *`
- 内部辅助函数 `_calc_count()`、`_get_item_ptr()` 参数类型同步更新
- `main.c` 测试代码中局部变量类型同步更新为 `ringbuf_cnt_t`

### 🧹 清理

- 移除因参数改为无符号类型后不再需要的 `< 0` 冗余检查

---

## [v1.0.0] - 初始版本

### ✨ 功能

- 支持不定长 `item_size` 的环形缓冲区实现
- 扩展索引范围（0~2\*depth-1），减少取模运算
- 覆盖/非覆盖两种写入模式
- 单个写入/读取：`ringBuf_push()` / `ringBuf_pop()`
- 按索引查看：`ringBuf_peek()`
- 批量操作：`ringBuf_push_multi()` / `ringBuf_pop_multi()` / `ringBuf_peek_multi()`
- 零拷贝设计，无动态内存分配
- 完整的测试套件
- 宏 `RINGBUFCRTL_INIT()` 用于静态初始化
- 调试宏支持（`RING_DEBUG`）
