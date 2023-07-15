#include "syscall.hpp"

usize strlen(const char *str) {
    usize len = 0;
    while (str[len])
        len++;
    return len;
}

void sys_debug(const char *string, u64 length) {
    syscall(0, (u64)string, length);
}

void sys_exit(int status) {
    syscall(1, status);
}

void print(const char *str) {
    sys_debug(str, strlen(str));
}

int main() {
    print("Hello from userspace!\n");
    print("I am printing this stuff with syscalls now :)\n");
    return 0;
}

extern "C" void _start() {
    sys_exit(main());
}
