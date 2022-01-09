#pragma once

#include <types.hpp>

namespace io {

static inline void outb(const u16 port, const u8 val) {
    asm volatile("outb %0, %1" : : "a" (val), "Nd" (port));
}

static inline void outw(const u16 port, const u16 val) {
    asm volatile("outw %0, %1" : : "a" (val), "Nd" (port));
}

static inline void outl(const u16 port, const u32 val) {
    asm volatile("outl %0, %1" : : "a" (val), "Nd" (port));
}

static inline u8 inb(const u16 port) {
    volatile u8 ret;
    asm volatile("inb %1, %0" : "=a" (ret) : "Nd" (port));
    return ret;
}

static inline u16 inw(const u16 port) {
    volatile u16 ret;
    asm volatile("inw %1, %0" : "=a" (ret) : "Nd" (port));
    return ret;
}

static inline u32 inl(const u16 port) {
    volatile u32 ret;
    asm volatile("inl %1, %0" : "=a" (ret) : "Nd" (port));
    return ret;
}

static inline void wait() {
    outb(0x80, 0); 
}

}
