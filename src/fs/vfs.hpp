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
        FileSystem *fs;
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
        klib::HashMap<Node*> children;

        DirectoryNode(FileSystem *fs, Node *parent, const char *name);
        virtual ~DirectoryNode();

        void create_dotentries();
    };

    struct DeviceNode : public Node {
        
    };

    struct FileSystem {
        Node* (*create)(FileSystem *fs, DirectoryNode *parent, const char *name, vfs::Node::Type type);
        isize (*read)(FileSystem *fs, Node *node, void *buf, usize count, usize offset);
        void (*write)(FileSystem *fs, Node *node, const void *buf, usize count, usize offset);
    };

    struct FileSystemDriver {
        FileSystem* (*instantiate)(FileSystemDriver *driver);
    };

    void init();
    void register_filesystem(const char *identifier, FileSystemDriver *fs_driver);
    DirectoryNode* root_dir();
    Node* reduce_node(Node *node, bool follow_symlinks);

    struct PathToNodeResult {
        DirectoryNode *target_parent;
        Node *target;
        char *basename; // make sure to delete[] basename!!!!!!!!!!
    };
    PathToNodeResult path_to_node(const char *path, DirectoryNode *starting_point = nullptr);
    
    struct FileDescriptor {
        fs::vfs::Node *node;
        usize cursor;
    };

    int syscall_open(const char *path);
    int syscall_openat(int dirfd, const char *path);
    void syscall_close(int fd);
    void syscall_read(int fd, void *buf, usize count);
    void syscall_pread(int fd, void *buf, usize count, usize offset);
    void syscall_write(int fd, const void *buf, usize count);
    void syscall_pwrite(int fd, const void *buf, usize count, usize offset);
    void syscall_seek(int fd, isize offset);
    isize syscall_getcwd(char *buf, usize size);
    isize syscall_chdir(const char *path);
}
