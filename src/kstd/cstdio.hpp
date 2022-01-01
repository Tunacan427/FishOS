#pragma once

#include <stdarg.h>
#include <types.hpp>

namespace kstd {

int putchar(char c);
int vprintf(const char *format, va_list list);
[[gnu::format(printf, 1, 2)]] int printf(const char *format, ...);

}
