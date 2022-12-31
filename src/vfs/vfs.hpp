#pragma once

#include <klib/types.hpp>
#include <klib/string.hpp>
#include <klib/hashmap.hpp>

namespace vfs {
    struct FileSystem {

    };

    struct Node {
        FileSystem *m_filesystem;
        klib::String m_name;
        Node *m_redirect;
        Node *m_parent;
        klib::HashMap<klib::String, Node*> *m_children;

        Node(FileSystem *fs, Node *parent, klib::String name, bool dir);

        void create_dotentries();
    };

    void init();
    void register_filesystem(klib::String &identifier, FileSystem *fs);
}
