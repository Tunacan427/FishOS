#pragma once

[[noreturn]] static inline void abort() {
    asm("cli");
    while (true) asm("hlt");
}

[[noreturn]] void panic(const char *format, ...);

#define ASSERT(x) do { if (!(x)) panic("Assertion failed in %s at %s:%d", __PRETTY_FUNCTION__, __FILE__, __LINE__); } while(false)
