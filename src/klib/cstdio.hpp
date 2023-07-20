#pragma once

#include <stdarg.h>
#include <klib/types.hpp>

namespace klib {
    int putchar(char c);
    int vprintf(const char *format, va_list list);
    [[gnu::format(printf, 1, 2)]] int printf(const char *format, ...);
    void panic_vprintf(const char *format, va_list list);
    [[gnu::format(printf, 1, 2)]] void panic_printf(const char *format, ...);
}
