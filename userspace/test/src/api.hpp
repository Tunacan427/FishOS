#pragma once

#include "types.hpp"
#include "posix.hpp"

int open(const char *path);
int openat(int dirfd, const char *path);
void close(int fd);
void read(int fd, void *buf, usize count);
void pread(int fd, void *buf, usize count, usize offset);
void write(int fd, const void *buf, usize count);
void pwrite(int fd, const void *buf, usize count, usize offset);
void seek(int fd, isize offset);
[[noreturn]] void exit(int status);
isize getcwd(char *buf, usize size);
isize chdir(const char *path);
isize mmap(void *hint, usize length, int prot, int flags, int fd, usize offset);
