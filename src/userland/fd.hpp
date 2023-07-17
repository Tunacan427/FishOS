#pragma once

#include <fs/vfs.hpp>

namespace userland {
    struct FileDescriptor {
        fs::vfs::Node *node;
        usize cursor;
    };

    void syscall_read(int fd, void *buf, usize count);
    void syscall_pread(int fd, void *buf, usize count, usize offset);
    void syscall_write(int fd, const void *buf, usize count);
    void syscall_pwrite(int fd, const void *buf, usize count, usize offset);
}
