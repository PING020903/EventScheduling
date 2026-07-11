# Changelog

All notable changes to this project will be documented in this file.

---

## [v1.2.0] - 2026-06-26

### ✨ Added

- **Enhanced parameter validation**: `RINGBUF_ARG_CHECK` macro now validates non-zero `depth`/`item_size` and prevents `2*depth` overflow of `ringbuf_uidx_t`, applied to all API entry points (`init`/`push`/`pop`/`peek`/`count`/`clear`)
- **Test cases**: Added Test 6 for parameter validation, covering `depth=0`, `item_size=0`, oversized buffer and other edge cases

### 🔄 Changed

- `ringBuf_init()` removed duplicate parameter checks, now unified under `RINGBUF_ARG_CHECK` macro

---

## [v1.1.0] - 2026-06-26

### 🛠 Fixed

- **Type overflow issue**: Introduced type aliases `ringbuf_uidx_t`, `ringbuf_ucnt_t`, `ringbuf_idx_t`, `ringbuf_cnt_t` to replace raw `int`/`short`/`unsigned short`, resolving potential overflow risks for index and count variables in large-depth buffer scenarios

### 🔄 Changed

- `wr_idx` and `rd_idx` in `ringbuf_t` struct changed to `ringbuf_uidx_t` (`unsigned int`)
- `depth` and `item_size` in `ringbuf_t` struct changed to `ringbuf_ucnt_t` (`unsigned short`)
- `ringBuf_peek()` parameter `itemIdx` type changed from `short` to `ringbuf_ucnt_t`
- `ringBuf_push_multi()` / `ringBuf_pop_multi()`: `dataCount` changed to `ringbuf_ucnt_t`, `pCount` changed to `ringbuf_cnt_t *`
- `ringBuf_peek_multi()`: `dataCount` changed to `ringbuf_ucnt_t`, `itemIdx` changed to `ringbuf_cnt_t`, `pCount` changed to `ringbuf_cnt_t *`
- Internal helpers `_calc_count()` and `_get_item_ptr()` parameter types updated accordingly
- Local variable types in `main.c` test code updated to `ringbuf_cnt_t`

### 🧹 Cleaned

- Removed redundant `< 0` checks no longer needed after parameters became unsigned types

---

## [v1.0.0] - Initial Release

### ✨ Features

- Ring buffer implementation with variable `item_size` support
- Extended index range (0~2\*depth-1) to reduce modulo operations
- Overwrite and non-overwrite write modes
- Single push/pop: `ringBuf_push()` / `ringBuf_pop()`
- Indexed peek: `ringBuf_peek()`
- Batch operations: `ringBuf_push_multi()` / `ringBuf_pop_multi()` / `ringBuf_peek_multi()`
- Zero-copy design with no dynamic memory allocation
- Complete test suite
- Macro `RINGBUFCRTL_INIT()` for static initialization
- Debug macro support (`RING_DEBUG`)
