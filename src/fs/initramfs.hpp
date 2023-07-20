#pragma once

#include <fs/vfs.hpp>

// not an actual fs, just a loader
namespace fs::initramfs {
    struct [[gnu::packed]] USTARHeader {
        char name[100];
        char mode[8];
        char uid[8];
        char gid[8];
        char size[12];
        char mtime[12];
        char chksum[8];
        char typeflag;
        char linkname[100];
        char magic[6];
        char version[2];
        char uname[32];
        char gname[32];
        char devmajor[8];
        char devminor[8];
        char prefix[155];
    };

    void load_into(vfs::DirectoryNode *dir, void *file_addr, usize file_size);
};
