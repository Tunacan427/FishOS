#pragma once

#include <klib/types.hpp>
#include <klib/hashmap.hpp>

namespace vfs {
    class FileSystem {
        
    };

    class FileSystemDriver {
	    virtual FileSystem* instantiate();
    };

    class Node {
    public:
        enum class Type {
            NONE,
            FILE,
            DIRECTORY
        };

        Type type;
        FileSystem *filesystem;
        Node *redirect;
        Node *parent;
        const char *name;

        Node(Type type, FileSystem *fs, Node *parent, const char *name);
        virtual ~Node();
    };

    class FileNode : public Node {
    public:
        FileNode(FileSystem *fs, Node *parent, const char *name);
        virtual ~FileNode();
    };

    class DirectoryNode : public Node {
        klib::HashMap<const char*, Node*> m_children;

        void create_dotentries();

    public:
        DirectoryNode(FileSystem *fs, Node *parent, const char *name);
        virtual ~DirectoryNode();
    };

    class DeviceNode : public Node {
        
    };

    void init();
    void register_filesystem(const char *identifier, FileSystemDriver *fs_driver);
}
