#include "syscall.hpp"
#include "print.hpp"
#include "cstring.hpp"

usize strlen(const char *str) {
    usize len = 0;
    while (str[len])
        len++;
    return len;
}

int main() {
    printf("Hello from userspace!\n");
    printf("Address of main: %#lX\n", (uptr)&main);
    {
        const char *text = "hello world this is a long string that a fish will put through a file descriptor into this file that exists in this tempfs ok bye lol\n";
        syscall(SYS_PWRITE, 3, (uptr)text, strlen(text), 0);
    }
    {
        u8 read_buffer[6];
        syscall(SYS_PREAD, 3, (uptr)read_buffer, 6, 39);
        printf("%.*s\n", 6, read_buffer);
    }
    printf("echo time\n");
    printf("type 'exit' to exit\n");
    while (true) {
        printf("> ");
        flush_print_buffer();
        u8 stdin_buffer[256] = {};
        stdin_buffer[255] = 0;
        syscall(SYS_READ, stdin, (uptr)stdin_buffer, 255);
        char *input = (char*)stdin_buffer;
        int len = strlen(input);
        if (std::strcmp(input, "exit\n") == 0) {
            printf("goodbye\n");
            return 0;
        }
        printf("%.*s", len, input);
    }
    return 0;
}

void sys_exit(int status) {
    syscall(SYS_EXIT, status);
}

extern "C" void _start() {
    int status = main();
    flush_print_buffer();
    sys_exit(status);
}
