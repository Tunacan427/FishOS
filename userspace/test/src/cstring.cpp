#include "cstring.hpp"

namespace std {
    int memcmp(const void *lhs, const void *rhs, usize size) {
        const u8 *a = (const u8*)lhs;
        const u8 *b = (const u8*)rhs;
        for (usize i = 0; i < size; i++) {
            if (a[i] < b[i])
                return -1;
            else if (b[i] < a[i])
                return 1;
        }
        return 0;
    }

    void* memcpy(void *dstptr, const void *srcptr, usize size) {
        u8 *dst = (u8*)dstptr;
        const u8 *src = (const u8*)srcptr;
        for (usize i = 0; i < size; i++)
            dst[i] = src[i];
        return dstptr;
    }

    void* memmove(void *dstptr, const void *srcptr, usize size) {
        u8 *dst = (u8*)dstptr;
        const u8 *src = (const u8*)srcptr;
        if (dst < src) {
            for (usize i = 0; i < size; i++)
                dst[i] = src[i];
        } else {
            for (usize i = size; i != 0; i--)
                dst[i-1] = src[i-1];
        }
        return dstptr;
    }

    void* memset(void *dstptr, u8 value, usize size) {
        u8 *dst = (u8*)dstptr;
        for (usize i = 0; i < size; i++)
            dst[i] = (u8)value;
        return dstptr;
    }

    usize strlen(const char *str) {
        usize len = 0;
        while (str[len])
            len++;
        return len;
    }

    char* strcpy(char *dst, const char *src) {
        return (char*)memcpy(dst, src, strlen(src) + 1);
    }

    // char* strdup(const char *src) {
    //     usize size = strlen(src) + 1;
    //     char *str = (char*)malloc(size);
    //     if (str)
    //         memcpy(str, src, size);
    //     return str;
    // }

    char* strcat(char *dst, const char *src) {
        strcpy(dst + strlen(dst), src);
        return dst;
    }

    int strcmp(const char *lhs, const char *rhs) {
        while (*lhs && (*lhs == *rhs)) {
            lhs++;
            rhs++;
        }
        return *(const unsigned char*)lhs - *(const unsigned char*)rhs;
    }
    
    int strncmp(const char *lhs, const char *rhs, usize count) {
        while (count && *lhs && (*lhs == *rhs)) {
            lhs++;
            rhs++;
            count--;
        }
        return count ? (*(const unsigned char*)lhs - *(const unsigned char*)rhs) : 0;
    }
}
