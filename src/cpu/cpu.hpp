#pragma once

#include <types.hpp>
#include <cpuid.h>

namespace cpu {
    static inline void cpuid(u32 leaf, u32 subleaf, u32 *eax, u32 *ebx, u32 *ecx, u32 *edx) {
        asm volatile("cpuid" : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx) : "a" (leaf), "c" (subleaf));
    }

    static inline u64 read_msr(u32 msr) {
        volatile u32 lo, hi;
        asm volatile("rdmsr" : "=a" (lo), "=d" (hi) : "c" (msr));
        return ((u64)hi << 32) | lo;
    }
    
    static inline void write_msr(u32 msr, u64 val) {
        volatile u32 lo = val & 0xFFFFFFFF;
        volatile u32 hi = val >> 32;
        asm volatile("wrmsr" : : "a" (lo), "d" (hi), "c" (msr));
    }

    static inline void write_cr3(u64 cr3) {
        asm volatile("mov %0, %%cr3" : : "r" (cr3));
    }

    static inline void write_cr4(u64 cr4) {
        asm volatile("mov %0, %%cr4" : : "r" (cr4));
    }

    static inline u64 read_cr4() {
        volatile u64 cr4;
        asm volatile("mov %%cr4, %0" : "=r" (cr4));
        return cr4;
    }

    static inline u64 read_cr2() {
        volatile u64 cr2;
        asm volatile("mov %%cr2, %0" : "=r" (cr2));
        return cr2;
    }

    static inline u32 bswap(u32 val) {
        volatile u32 result;
        asm volatile("bswap %0" : "=r" (result) : "r" (val));
        return result;
    }

    static inline u64 bswap(u64 val) {
        volatile u64 result;
        asm volatile("bswap %0" : "=r" (result) : "r" (val));
        return result;
    }
}
