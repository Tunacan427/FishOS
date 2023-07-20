#pragma once

#include <klib/types.hpp>

namespace klib {
    // mem functions are implemented in mem.asm
    extern "C" int memcmp(const void *lhs, const void *rhs, usize size);
    extern "C" void* memcpy(void *dst, const void *src, usize size);
    extern "C" void* memmove(void *dst, const void *src, usize size);
    extern "C" void* memset(void *dst, u8 value, usize size);

    usize strlen(const char *str);
    char* strcpy(char *dst, const char *src);
    char* strdup(const char *src);
    char* strcat(char *dst, const char *src);
    int strcmp(const char *lhs, const char *rhs);
    int strncmp(const char *lhs, const char *rhs, usize count);
    char* strchr(char *str, char c);
    const char* strchr(const char *str, char c);
}
