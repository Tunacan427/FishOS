#pragma once

#include "types.hpp"

namespace klib {
    int memcmp(const void *lhs, const void *rhs, usize size);
    void* memcpy(void *dst, const void *src, usize size);
    void* memmove(void *dst, const void *src, usize size);
    void* memset(void *dst, u8 value, usize size);
    usize strlen(const char *str);
    char* strcpy(char *dst, const char *src);
    char* strcat(char *dst, const char *src);
    int strcmp(const char *lhs, const char *rhs);
    int strncmp(const char *lhs, const char *rhs, usize count);
}
