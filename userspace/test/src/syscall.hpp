#pragma once

#include "types.hpp"

#define SYS_exit   0
#define SYS_open   1
#define SYS_openat 2
#define SYS_close  3
#define SYS_read   4
#define SYS_pread  5
#define SYS_write  6
#define SYS_pwrite 7
#define SYS_seek   8
#define SYS_getcwd 9
#define SYS_chdir  10

#define SYSCALL_INLINE [[gnu::always_inline]] inline

SYSCALL_INLINE u64 syscall(u64 sc) {
    u64 ret;
    asm volatile("syscall" : "=a" (ret)
            : "a" (sc)
            : "rcx", "r11", "memory");
    return ret;
}

SYSCALL_INLINE u64 syscall(u64 sc, u64 arg1) {
    u64 ret;
    asm volatile("syscall" : "=a" (ret)
            : "a" (sc), "D" (arg1)
            : "rcx", "r11", "memory");
    return ret;
}

SYSCALL_INLINE u64 syscall(u64 sc, u64 arg1, u64 arg2) {
    u64 ret;
    asm volatile("syscall" : "=a" (ret)
            : "a" (sc), "D" (arg1), "S" (arg2)
            : "rcx", "r11", "memory");
    return ret;
}

SYSCALL_INLINE u64 syscall(u64 sc, u64 arg1, u64 arg2, u64 arg3) {
    u64 ret;
    asm volatile("syscall" : "=a" (ret)
            : "a" (sc), "D" (arg1), "S" (arg2), "d" (arg3)
            : "rcx", "r11", "memory");
    return ret;
}

SYSCALL_INLINE u64 syscall(u64 sc, u64 arg1, u64 arg2, u64 arg3, u64 arg4) {
    u64 ret;
    register u64 arg4_reg asm("r10") = arg4;
    asm volatile("syscall" : "=a" (ret)
            : "a" (sc), "D" (arg1), "S" (arg2), "d" (arg3), "r" (arg4_reg)
            : "rcx", "r11", "memory");
    return ret;
}

SYSCALL_INLINE u64 syscall(u64 sc, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5) {
    u64 ret;
    register u64 arg4_reg asm("r10") = arg4;
    register u64 arg5_reg asm("r8")  = arg5;
    asm volatile("syscall" : "=a" (ret)
            : "a" (sc), "D" (arg1), "S" (arg2), "d" (arg3), "r" (arg4_reg), "r" (arg5_reg)
            : "rcx", "r11", "memory");
    return ret;
}

SYSCALL_INLINE u64 syscall(u64 sc, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5, u64 arg6) {
    u64 ret;
    register u64 arg4_reg asm("r10") = arg4;
    register u64 arg5_reg asm("r8")  = arg5;
    register u64 arg6_reg asm("r9")  = arg6;
    asm volatile("syscall" : "=a" (ret)
            : "a" (sc), "D" (arg1), "S" (arg2), "d" (arg3), "r" (arg4_reg), "r" (arg5_reg), "r" (arg6_reg)
            : "rcx", "r11", "memory");
    return ret;
}
