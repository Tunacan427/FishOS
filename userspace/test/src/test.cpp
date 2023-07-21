#include "api.hpp"
#include "print.hpp"
#include "string.hpp"

static void test_cwd() {
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
}

static void test_openat() {
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
}

static void test_fd() {
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
}

static void test_mmap() {
    {
        printf("mmapping 64 MiB of memory and writing it\n");
        constexpr int N = 1024 * 1024 * 16;
        isize ret = mmap(nullptr, N * sizeof(int), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
        if (ret < 0) {
            printf("mmap fail\n");
            return;
        }
        int *ptr = (int*)ret;

        // fill the elements of the array
        for (int i = 0; i < N; i++)
            ptr[i] = i * 10;

        // print the elements of the array
        printf("checking\n");
        for (int i = 0; i < N; i++) {
            if (ptr[i] != i * 10) {
                printf("incorrect\n");
                return;
            }
        }

        printf("done\n");
    }
    {
        printf("mmapping 1 GiB of memory and writing only 4 MiB of it\n");
        isize ret = mmap(nullptr, 1024 * 1024 * 1024, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
        if (ret < 0) {
            printf("mmap fail\n");
            return;
        }
        int *ptr = (int*)ret;

        constexpr int N = 1024 * 1024 * 1;
        // fill the elements of the array
        for (int i = N * 5; i < N * 6; i++)
            ptr[i] = i * 10;

        // print the elements of the array
        printf("checking\n");
        for (int i = N * 5; i < N * 6; i++) {
            if (ptr[i] != i * 10) {
                printf("incorrect\n");
                return;
            }
        }

        printf("done\n");
    }
}

int main() {
    printf("Hello from userspace!\n");
    printf("Address of main: %#lX\n", (uptr)&main);
    printf("Type 'help' to list tests\n");
    while (true) {
        printf("> ");
        flush_print_buffer();
        u8 stdin_buffer[256] = {};
        stdin_buffer[255] = 0;
        read(stdin, stdin_buffer, 255);
        char *input = (char*)stdin_buffer;
        if (strcmp(input, "help\n") == 0)   { printf("exit\ncwd\nopenat\nfd\nmmap\n"); continue; }
        if (strcmp(input, "exit\n") == 0)   { printf("goodbye\n"); return 0; }
        if (strcmp(input, "cwd\n") == 0)    { test_cwd(); continue; }
        if (strcmp(input, "openat\n") == 0) { test_openat(); continue; }
        if (strcmp(input, "fd\n") == 0)     { test_fd(); continue; }
        if (strcmp(input, "mmap\n") == 0)   { test_mmap(); continue; }
        printf("invalid command\n");
    }
    return 0;
}

extern "C" [[noreturn]] void _start() {
    int status = main();
    flush_print_buffer();
    exit(status);
}
