#include <userland/fd.hpp>
#include <klib/cstdio.hpp>
#include <sched/sched.hpp>
#include <ps2/kbd/keyboard.hpp>
#include <cpu/syscall/syscall.hpp>

namespace userland {
    void syscall_read(int fd, void *buf, usize count) {
#if SYSCALL_TRACE
        klib::printf("read(%d, %#lX, %ld)\n", fd, (uptr)buf, count);
#endif
        auto *task = (sched::Task*)cpu::read_gs_base();
        if (fd >= (int)task->file_descriptors.size() || fd < 0) return;
        if (fd == 0) {
            ps2::kbd::read(buf, count);
            return;
        }
        FileDescriptor *descriptor = &task->file_descriptors[fd];
        if (descriptor == nullptr) return;
        fs::vfs::FileSystem *fs = descriptor->node->filesystem;
        fs->read(fs, descriptor->node, buf, count, descriptor->cursor);
        descriptor->cursor += count;
    }

    void syscall_pread(int fd, void *buf, usize count, usize offset) {
#if SYSCALL_TRACE
        klib::printf("pread(%d, %#lX, %ld, %ld)\n", fd, (uptr)buf, count, offset);
#endif
        auto *task = (sched::Task*)cpu::read_gs_base();
        if (fd >= (int)task->file_descriptors.size() || fd < 0) return;
        FileDescriptor *descriptor = &task->file_descriptors[fd];
        if (descriptor == nullptr) return;
        fs::vfs::FileSystem *fs = descriptor->node->filesystem;
        fs->read(fs, descriptor->node, buf, count, offset);
    }

    void syscall_write(int fd, const void *buf, usize count) {
#if SYSCALL_TRACE
        klib::printf("write(%d, %#lX, %ld)\n", fd, (uptr)buf, count);
#endif
        auto *task = (sched::Task*)cpu::read_gs_base();
        if (fd >= (int)task->file_descriptors.size() || fd < 0) return;
        if (fd == 1) {
            klib::printf("%.*s", (int)count, (char*)buf);
            return;
        }
        FileDescriptor *descriptor = &task->file_descriptors[fd];
        if (descriptor == nullptr) return;
        fs::vfs::FileSystem *fs = descriptor->node->filesystem;
        fs->write(fs, descriptor->node, buf, count, descriptor->cursor);
        descriptor->cursor += count;
    }

    void syscall_pwrite(int fd, const void *buf, usize count, usize offset) {
#if SYSCALL_TRACE
        klib::printf("pwrite(%d, %#lX, %ld, %ld)\n", fd, (uptr)buf, count, offset);
#endif
        auto *task = (sched::Task*)cpu::read_gs_base();
        if (fd >= (int)task->file_descriptors.size() || fd < 0) return;
        FileDescriptor *descriptor = &task->file_descriptors[fd];
        if (descriptor == nullptr) return;
        fs::vfs::FileSystem *fs = descriptor->node->filesystem;
        fs->write(fs, descriptor->node, buf, count, offset);
    }
}
