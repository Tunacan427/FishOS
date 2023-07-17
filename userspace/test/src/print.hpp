#pragma once

#include <stdarg.h>

#define stdin  0
#define stdout 1
#define stderr 2

void flush_print_buffer();
int putchar(char c);
void puts(const char *str);
int vprintf(const char *format, va_list list);
[[gnu::format(printf, 1, 2)]] int printf(const char *format, ...);
