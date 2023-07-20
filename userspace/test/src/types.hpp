#pragma once

#include <stdint.h>
#include <stddef.h>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using usize = size_t;
using isize = i64; // warning!!!!!!!!!!!!!!!!!!!!
using uptr = uintptr_t;

using nullptr_t = decltype(nullptr);
