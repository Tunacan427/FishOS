#pragma once

#include <klib/types.hpp>
#include <klib/hashmap.hpp>

namespace fs::vfs {
    struct FileSystem;
    struct Node {
        enum class Type {
            NONE,
            FILE,
            DIRECTORY
        };

        Type type;
        FileSystem *filesystem;
        void *data; // used by the filesystem
        Node *redirect;
        Node *parent;
        const char *name;

        Node(Type type, FileSystem *fs, Node *parent, const char *name);
        virtual ~Node();
    };

    struct FileNode : public Node {
        FileNode(FileSystem *fs, Node *parent, const char *name);
        virtual ~FileNode();
    };

    struct DirectoryNode : public Node {
        klib::HashMap<klib::String, Node*> children;

        DirectoryNode(FileSystem *fs, Node *parent, const char *name);
        virtual ~DirectoryNode();

        void create_dotentries();
    };

    struct DeviceNode : public Node {
        
    };

    struct FileSystem {
        Node* (*create)(FileSystem *fs, DirectoryNode *parent, const char *name, vfs::Node::Type type);
        void (*read)(FileSystem *fs, Node *node, void *buf, usize count, usize offset);
        void (*write)(FileSystem *fs, Node *node, const void *buf, usize count, usize offset);
    };

    struct FileSystemDriver {
        FileSystem* (*instantiate)(FileSystemDriver *driver);
    };

    void init();
    void register_filesystem(const char *identifier, FileSystemDriver *fs_driver);
    DirectoryNode* root_dir();
}
