#include <fs/vfs.hpp>
#include <klib/lock.hpp>

namespace vfs {
    static klib::Spinlock vfs_lock;
    static Node *root = nullptr;

    static auto fs_drivers() {
        static klib::HashMap<const char*, FileSystemDriver*> file_systems;
        return file_systems;
    }

    void register_filesystem(const char *identifier, FileSystemDriver *fs_driver) {
        klib::LockGuard guard(vfs_lock);
        fs_drivers().insert(identifier, fs_driver);
    }

    void init() {
        fs_drivers();
    }

    static Node* reduce_node(Node *node, bool follow_symlinks) {
        if (node->redirect)
            return reduce_node(node->redirect, follow_symlinks);
        return node;
    }
    
    static Node* node_from_path(char *path) {
        usize path_len = klib::strlen(path);

        if (path_len == 0 || path[0] != '/') return nullptr;
        Node *current_node = root;

        for (usize i = 0; i < path_len; i++) {
            if (path[i] == '/') continue;
            
        }
        
        return nullptr;
    }

    Node::Node(Type type, FileSystem *fs, Node *parent, const char *name) : type(type), filesystem(fs), parent(parent), name(name) {}
    Node::~Node() {}

    FileNode::FileNode(FileSystem *fs, Node *parent, const char *name) : Node(Type::FILE, fs, parent, name) {}
    FileNode::~FileNode() {}
    
    DirectoryNode::DirectoryNode(FileSystem *fs, Node *parent, const char *name) : Node(Type::DIRECTORY, fs, parent, name) {}
    DirectoryNode::~DirectoryNode() {}

    void DirectoryNode::create_dotentries() {
        auto dot = new Node(Type::NONE, filesystem, this, ".");
        auto dotdot = new Node(Type::NONE, filesystem, this, "..");
        dot->redirect = this;
        dotdot->redirect = parent;
        m_children.insert(".", dot);
        m_children.insert("..", dotdot);
    }
}
