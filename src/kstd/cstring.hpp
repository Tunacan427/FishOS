#pragma once

#include <types.hpp>

namespace kstd {

int memcmp(const void *aptr, const void *bptr, usize size);
void* memcpy(void *dstptr, const void *srcptr, usize size);
void* memmove(void *dstptr, const void *srcptr, usize size);
void* memset(void *bufptr, int value, usize size);
usize strlen(const char *str);
char* strcpy(char *dst, const char *src);
char* strcat(char *dst, const char *src);

}
