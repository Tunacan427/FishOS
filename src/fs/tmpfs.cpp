#include <fs/tmpfs.hpp>

namespace fs::tmpfs {
    vfs::Node* create(vfs::FileSystem *fs, vfs::DirectoryNode *parent, const char *name, vfs::Node::Type type) {
        switch (type) {
        case vfs::Node::Type::FILE: {
            auto *node = new vfs::FileNode(fs, parent, name);
            NodeData *data = new NodeData();
            data->size = 0;
            data->storage = nullptr;
            node->data = data;
            parent->children.insert(name, node);
            return node;
        }
        case vfs::Node::Type::DIRECTORY: {
            auto *node = new vfs::DirectoryNode(fs, parent, name);
            node->create_dotentries();
            if (parent)
                parent->children.insert(name, node);
            return node;
        }
        default:
            return nullptr;
        }
    }

    isize read(vfs::FileSystem *fs, vfs::Node *node, void *buf, usize count, usize offset) {
        if (node->type != vfs::Node::Type::FILE) return -1;
        NodeData *data = (NodeData*)node->data;
        if (offset >= data->size)
            return 0;
        if (offset + count > data->size) { // partial read
            usize actual_count = data->size - offset;
            klib::memcpy(buf, data->storage + offset, actual_count);
            return actual_count;
        }
        klib::memcpy(buf, data->storage + offset, count);
        return count;
    }

    void write(vfs::FileSystem *fs, vfs::Node *node, const void *buf, usize count, usize offset) {
        if (node->type != vfs::Node::Type::FILE) return;
        NodeData *data = (NodeData*)node->data;
        if (offset + count > data->size) {
            data->size = offset + count;
            data->storage = (u8*)klib::realloc(data->storage, data->size);
        }
        klib::memcpy(data->storage + offset, buf, count);
    }

    vfs::FileSystem* instantiate(vfs::FileSystemDriver *driver) {
        auto *fs = new vfs::FileSystem();
        fs->create = &create;
        fs->read = &read;
        fs->write = &write;
        return fs;
    }

    vfs::FileSystemDriver* create_driver() {
        auto *driver = new vfs::FileSystemDriver();
        driver->instantiate = &instantiate;
        return driver;
    }
}
