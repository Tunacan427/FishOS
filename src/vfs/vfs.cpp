#include <vfs/vfs.hpp>
#include <klib/lock.hpp>

namespace vfs {
    static klib::Spinlock vfs_lock;
    static Node *root = nullptr;

    static auto file_systems() {
        static klib::HashMap<klib::String, FileSystem*> file_systems;
        return file_systems;
    }

    void register_filesystem(klib::String &identifier, FileSystem *fs) {
        klib::LockGuard guard(vfs_lock);
        file_systems().insert(identifier, fs);
    }

    void init() {
        file_systems();
        root = new Node(nullptr, nullptr, "", false);
    }

    static Node* reduce_node(Node *node, bool follow_symlinks) {
        if (node->m_redirect)
            return reduce_node(node->m_redirect, follow_symlinks);
        return node;
    }
    
    static Node* node_from_path(klib::String &path) {
        if (path.length() == 0 || path[0] != '/') return nullptr;
        Node *current_node = root;

        for (usize i = 0; i < path.length(); i++) {
            if (path[i] == '/') continue;
            
        }
        
        return nullptr;
    }

    Node::Node(FileSystem *fs, Node *parent, klib::String name, bool dir) : m_filesystem(fs), m_name(name), m_parent(parent) {
        if (dir)
            m_children = new klib::HashMap<klib::String, Node*>();
    }

    void Node::create_dotentries() {
        auto dot = new Node(m_filesystem, this, ".", false);
        auto dotdot = new Node(m_filesystem, this, "..", false);
        dot->m_redirect = this;
        dotdot->m_redirect = m_parent;
        m_children->insert(".", dot);
        m_children->insert("..", dotdot);
    }
}
