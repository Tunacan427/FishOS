#include "print.hpp"
#include "api.hpp"

extern "C" {
    uptr __stack_chk_guard = 0xED1A449A97A8154E;
    [[noreturn]] void __stack_chk_fail() {
        printf("\nStack smashing detected\n");
        exit(1);
    }
}
