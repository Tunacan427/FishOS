#include <klib/cstring.hpp>
#include <klib/cstdlib.hpp>

namespace klib {
    usize strlen(const char *str) {
        usize len = 0;
        while (str[len])
            len++;
        return len;
    }

    char* strcpy(char *dst, const char *src) {
        return (char*)memcpy(dst, src, strlen(src) + 1);
    }

    char* strdup(const char *src) {
        usize size = strlen(src) + 1;
        char *str = (char*)malloc(size);
        if (str)
            memcpy(str, src, size);
        return str;
    }

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

    char* strchr(char *str, char c) {
        while (*str != c)
            if (*str++ == 0)
                return nullptr;
        return str;
    }

    const char* strchr(const char *str, char c) {
        while (*str != c)
            if (*str++ == 0)
                return nullptr;
        return str;
    }
}
