#include <fs/initramfs.hpp>
#include <klib/cstdio.hpp>
#include <klib/algorithm.hpp>
#include <panic.hpp>

namespace fs::initramfs {
    static usize octal_to_bin(const char *str, usize len) {
        usize value = 0;
        while (*str && len > 0) {
            value = value * 8 + (*str++ - '0');
            len--;
        }
        return value;
    }

    void load_into(vfs::DirectoryNode *dir, void *file_addr, usize file_size) {
        USTARHeader *current = (USTARHeader*)file_addr;
        while (true) {
            if (klib::memcmp(current->magic, "ustar", 6) != 0)
                break;
            
            usize size = octal_to_bin(current->size, sizeof(current->size));

            klib::printf("Name: %s, size: ", current->name);
            if (size > 1024) klib::printf("%ld KiB\n", size / 1024);
            else klib::printf("%ld B\n", size);

            if (klib::strcmp(current->name, "./") != 0) {
                auto result = vfs::path_to_node(current->name, dir);
                ASSERT(result.target == nullptr);
                ASSERT(result.basename != nullptr);

                switch (current->typeflag) {
                case '0':
                case '\0': { // file
                    auto *file = dir->fs->create(dir->fs, result.target_parent, result.basename, vfs::Node::Type::FILE);
                    file->fs->write(file->fs, file, (void*)((uptr)current + 512), size, 0);
                    break;
                }
                case '5': // directory
                    dir->fs->create(dir->fs, result.target_parent, result.basename, vfs::Node::Type::DIRECTORY);
                    break;
                default:
                    panic("Unimplemented node type in initramfs");
                }
                
                delete[] result.basename;
            }

            current = (USTARHeader*)((uptr)current + 512 + klib::align_up<usize, 512>(size));
            if ((uptr)current >= (uptr)file_addr + file_size)
                break;
        }
    }
}
