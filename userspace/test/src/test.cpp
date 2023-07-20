#include "api.hpp"
#include "print.hpp"
#include "string.hpp"

int main() {
    printf("Hello from userspace!\n");
    printf("Address of main: %#lX\n", (uptr)&main);
    {
        char cwd[64] = {};
        getcwd(cwd, 64);
        printf("cwd: %s\n", cwd);
    }
    chdir("./bin/");
    {
        char cwd[64] = {};
        getcwd(cwd, 64);
        printf("cwd: %s\n", cwd);

        int test_fd = open("test");
        char buf[4] = {};
        read(test_fd, buf, 4);
        if (memcmp(buf, "\177ELF", 4) == 0)
            printf("%s/test is indeed an ELF binary\n", cwd);
        close(test_fd);
    }
    chdir("..");
    {
        char cwd[64] = {};
        getcwd(cwd, 64);
        printf("cwd: %s\n", cwd);
    }
    int hello_fd = open("hello/");
    printf("fd %d is the hello directory\n", hello_fd);
    int world_fd = openat(hello_fd, "world.txt");
    {
        const char *text = "hello world this is a long string that a fish will put through a file descriptor into this file that exists in this tempfs ok bye lol\n";
        pwrite(world_fd, text, strlen(text), 0);
    }
    {
        u8 read_buffer[6] = {};
        pread(world_fd, read_buffer, 6, 39);
        printf("%.*s\n", 6, read_buffer);
    }
    close(world_fd);
    int test_fd = open("/./hello/.././.././test.txt");
    {
        const char *text1 = "Hello";
        write(test_fd, text1, strlen(text1));
        const char *text2 = ", ";
        write(test_fd, text2, strlen(text2));
        const char *text3 = "world!";
        write(test_fd, text3, strlen(text3));
    }
    {
        seek(test_fd, 0);
        u8 read_buffer[13] = {};
        read(test_fd, read_buffer, 13);
        printf("%.*s\n", 13, read_buffer);
    }
    close(test_fd);
    printf("echo time\n");
    printf("type 'exit' to exit\n");
    while (true) {
        printf("> ");
        flush_print_buffer();
        u8 stdin_buffer[256] = {};
        stdin_buffer[255] = 0;
        read(stdin, stdin_buffer, 255);
        char *input = (char*)stdin_buffer;
        int len = strlen(input);
        if (strcmp(input, "exit\n") == 0) {
            printf("goodbye\n");
            return 0;
        }
        printf("%.*s", len, input);
    }
    return 0;
}

extern "C" void _start() {
    int status = main();
    flush_print_buffer();
    exit(status);
}
