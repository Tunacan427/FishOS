#include "api.hpp"
#include "syscall.hpp"

int open(const char *path) {
    return syscall(SYS_open, (uptr)path);
}

int openat(int dirfd, const char *path) {
    return syscall(SYS_openat, dirfd, (uptr)path);
}

void close(int fd) {
    syscall(SYS_close, fd);
}

void read(int fd, void *buf, usize count) {
    syscall(SYS_read, fd, (uptr)buf, count);
}

void pread(int fd, void *buf, usize count, usize offset) {
    syscall(SYS_pread, fd, (uptr)buf, count, offset);
}

void write(int fd, const void *buf, usize count) {
    syscall(SYS_write, fd, (uptr)buf, count);
}

void pwrite(int fd, const void *buf, usize count, usize offset) {
    syscall(SYS_pwrite, fd, (uptr)buf, count, offset);
}

void seek(int fd, isize offset) {
    syscall(SYS_seek, fd, offset);
}

void exit(int status) {
    syscall(SYS_exit, status);
}

isize getcwd(char *buf, usize size) {
    return syscall(SYS_getcwd, (uptr)buf, size);
}

isize chdir(const char *path) {
    return syscall(SYS_chdir, (uptr)path);
}
