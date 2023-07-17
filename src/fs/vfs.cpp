#include <fs/vfs.hpp>
#include <fs/tmpfs.hpp>
#include <klib/lock.hpp>
#include <klib/cstdio.hpp>

namespace fs::vfs {
    static klib::Spinlock vfs_lock;
    static Node *root = nullptr;

    DirectoryNode* root_dir() { return (DirectoryNode*)root; }

    static auto fs_drivers() {
        static klib::HashMap<klib::String, FileSystemDriver*> file_systems;
        return file_systems;
    }

    void register_filesystem(const char *identifier, FileSystemDriver *fs_driver) {
        klib::LockGuard guard(vfs_lock);
        fs_drivers().insert(identifier, fs_driver);
    }

    void init() {
        fs_drivers();
        FileSystemDriver *tmpfs_driver = tmpfs::create_driver();
        register_filesystem("tmpfs", tmpfs_driver);

        FileSystem *root_fs = tmpfs_driver->instantiate(tmpfs_driver);
        root = root_fs->create(root_fs, nullptr, "", Node::Type::DIRECTORY);
        auto *root_dir = (DirectoryNode*)root;
        root_fs->create(root_fs, root_dir, "hello", Node::Type::FILE);
        root_fs->create(root_fs, root_dir, "world.txt", Node::Type::FILE);
        
        klib::printf("VFS: Listing / contents\n");
        root_dir->children.for_each([&] (klib::String *name, Node **node) {
            klib::printf("    %s\n", name->c_str());
        });
    }

    static Node* reduce_node(Node *node, bool follow_symlinks) {
        if (node->redirect)
            return reduce_node(node->redirect, follow_symlinks);
        return node;
    }
    
    static Node* node_from_path(const char *path) {
        usize path_len = klib::strlen(path);

        if (path_len == 0 || path[0] != '/') return nullptr;
        Node *current_node = root;

        usize previous_slash = 0;
        for (usize i = 0; i < path_len; i++) {
            if (path[i] == '/') {

                previous_slash = i;
                i++;
                continue;
            }
            
        }
        
        return nullptr;
    }

    Node::Node(Type type, FileSystem *fs, Node *parent, const char *name) : type(type), filesystem(fs), parent(parent), name(klib::strdup(name)) {}
    Node::~Node() {}

    FileNode::FileNode(FileSystem *fs, Node *parent, const char *name) : Node(Type::FILE, fs, parent, name) {}
    FileNode::~FileNode() {}
    
    DirectoryNode::DirectoryNode(FileSystem *fs, Node *parent, const char *name) : Node(Type::DIRECTORY, fs, parent, name) {}
    DirectoryNode::~DirectoryNode() {}

    void DirectoryNode::create_dotentries() {
        auto dot = new Node(Type::NONE, filesystem, this, ".");
        auto dotdot = new Node(Type::NONE, filesystem, this, "..");
        dot->redirect = this;
        dotdot->redirect = parent ? parent : this;
        children.insert(".", dot);
        children.insert("..", dotdot);
    }
}
