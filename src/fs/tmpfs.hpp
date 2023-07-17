#pragma once

#include <fs/vfs.hpp>

namespace fs::tmpfs {
    struct NodeData {
        u8 *storage;
        usize size;
    };

    vfs::FileSystemDriver* create_driver();
}
