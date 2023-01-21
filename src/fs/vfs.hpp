#pragma once

#include <klib/types.hpp>
#include <klib/hashmap.hpp>

namespace vfs {
    class FileSystem {
        
    };

    class FileSystemDriver {
	    virtual FileSystem* instantiate();
    };

    struct Node {
        enum class Type {
            NONE,
            FILE,
            DIRECTORY
        };

        Type m_type;
        FileSystem *m_filesystem;
        Node *m_redirect;
        Node *m_parent;
        const char *m_name;

        Node(Type type, FileSystem *fs, Node *parent, const char *name);
    };

    struct FileNode : Node {
        FileNode(FileSystem *fs, Node *parent, const char *name);
    };

    struct DirectoryNode : Node {
        klib::HashMap<const char*, Node*> m_children;

        DirectoryNode(FileSystem *fs, Node *parent, const char *name);

        void create_dotentries();
    };

    struct DeviceNode : Node {
        
    };

    void init();
    void register_filesystem(const char *identifier, FileSystemDriver *fs_driver);
}
