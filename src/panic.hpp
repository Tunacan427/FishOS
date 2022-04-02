#pragma once

[[noreturn]] static inline void abort() {
    asm("cli");
    while (true) asm("hlt");
}

[[noreturn]] void panic(const char *format, ...);
