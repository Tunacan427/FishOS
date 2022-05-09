#pragma once

#include <kstd/types.hpp>
#include <kstd/vector.hpp>
#include <kstd/string.hpp>
#include <kstd/functional.hpp>

namespace vfs {
    struct INode {
        u64 id;
    };

    struct FileOperations {
    };

    struct File {
        INode *inode;
        kstd::U8String name;
    };

    struct Directory {
        INode *inode;
    };
}
