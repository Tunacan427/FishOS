#include <fs/vfs.hpp>
#include <fs/tmpfs.hpp>
#include <klib/cstdio.hpp>
#include <klib/posix.hpp>
#include <sched/sched.hpp>
#include <ps2/kbd/keyboard.hpp>
#include <cpu/syscall/syscall.hpp>

namespace fs::vfs {
    static Node *root = nullptr;

    DirectoryNode* root_dir() { return (DirectoryNode*)root; }

    static auto fs_drivers() {
        static klib::HashMap<FileSystemDriver*> file_systems;
        return file_systems;
    }

    void register_filesystem(const char *identifier, FileSystemDriver *fs_driver) {
        fs_drivers().insert(identifier, fs_driver);
    }

    void init() {
        fs_drivers();
        FileSystemDriver *tmpfs_driver = tmpfs::create_driver();
        register_filesystem("tmpfs", tmpfs_driver);

        FileSystem *root_fs = tmpfs_driver->instantiate(tmpfs_driver);
        root = root_fs->create(root_fs, nullptr, "", Node::Type::DIRECTORY);
    }

    Node* reduce_node(Node *node, bool follow_symlinks) {
        if (node->redirect)
            return reduce_node(node->redirect, follow_symlinks);
        return node;
    }
    
    PathToNodeResult path_to_node(const char *path, DirectoryNode *starting_point) {
        usize path_len = klib::strlen(path);
        if (path_len == 0) return PathToNodeResult(nullptr, nullptr, nullptr);
        
        Node *current_node = starting_point;
        if (starting_point == nullptr) {
            if (path[0] != '/') return PathToNodeResult(nullptr, nullptr, nullptr);
            path++;
            path_len--;
            current_node = root;
        }
        
        while (true) {
            if (current_node->type != Node::Type::DIRECTORY)
                return PathToNodeResult(nullptr, nullptr, nullptr);
            auto *dir_node = (DirectoryNode*)current_node;
            bool last = false;
            const char *s = klib::strchr(path, '/');
            if (s == nullptr) {
                s = path + path_len;
                last = true;
            } else if (s == path + path_len - 1) {
                last = true;
            }

            usize entry_name_len = s - path;
            char *entry_name = new char[entry_name_len + 1];
            entry_name[entry_name_len] = 0;
            klib::memcpy(entry_name, path, entry_name_len);

            Node **next_node = dir_node->children.get(entry_name);
            if (next_node == nullptr)
                return PathToNodeResult(dir_node, nullptr, entry_name);

            if (last)
                return PathToNodeResult(dir_node, reduce_node(*next_node, false), entry_name);
            
            delete[] entry_name;
            path += entry_name_len + 1;
            path_len -= entry_name_len + 1;
            current_node = reduce_node(*next_node, false);
        }
        
        return PathToNodeResult(nullptr, nullptr, nullptr);
    }

    Node::Node(Type type, FileSystem *fs, Node *parent, const char *name) : type(type), fs(fs), parent(parent), name(klib::strdup(name)) {}
    Node::~Node() {}

    FileNode::FileNode(FileSystem *fs, Node *parent, const char *name) : Node(Type::FILE, fs, parent, name) {}
    FileNode::~FileNode() {}
    
    DirectoryNode::DirectoryNode(FileSystem *fs, Node *parent, const char *name) : Node(Type::DIRECTORY, fs, parent, name) {}
    DirectoryNode::~DirectoryNode() {}

    void DirectoryNode::create_dotentries() {
        auto dot = new Node(Type::NONE, fs, this, ".");
        auto dotdot = new Node(Type::NONE, fs, this, "..");
        dot->redirect = this;
        dotdot->redirect = parent ? parent : this;
        children.insert(".", dot);
        children.insert("..", dotdot);
    }

    static int openat_inner(int dirfd, const char *path) {
        auto *task = (sched::Task*)cpu::read_gs_base();
        DirectoryNode *starting_point = nullptr;
        if (path[0] != '/') { // path is relative
            if (dirfd == AT_FDCWD) {
                starting_point = task->cwd;
            } else {
                if (dirfd >= (int)task->file_descriptors.size() || dirfd < 0)
                    return -EBADF;
                FileDescriptor *descriptor = task->file_descriptors[dirfd];
                if (descriptor == nullptr)
                    return -EBADF;
                if (descriptor->node->type != Node::Type::DIRECTORY)
                    return -ENOTDIR;
                starting_point = (DirectoryNode*)descriptor->node;
            }
        }
        auto result = path_to_node(path, starting_point);
        if (result.basename != nullptr)
            delete[] result.basename;
        if (result.target == nullptr)
            return -ENOENT;
        int fd = task->allocate_fdnum();
        task->file_descriptors[fd] = new FileDescriptor(result.target, 0);
        return fd;
    }

    int syscall_open(const char *path) {
#if SYSCALL_TRACE
        klib::printf("open(\"%s\")\n", path);
#endif
        return openat_inner(AT_FDCWD, path);
    }

    int syscall_openat(int dirfd, const char *path) {
#if SYSCALL_TRACE
        klib::printf("openat(%d, \"%s\")\n", dirfd, path);
#endif
        return openat_inner(dirfd, path);
    }

    void syscall_close(int fd) {
#if SYSCALL_TRACE
        klib::printf("close(%d)\n", fd);
#endif
        auto *task = (sched::Task*)cpu::read_gs_base();
        if (fd >= (int)task->file_descriptors.size() || fd < 0) return;
        FileDescriptor *descriptor = task->file_descriptors[fd];
        if (descriptor == nullptr) return;
        delete task->file_descriptors[fd];
        task->file_descriptors[fd] = nullptr;
        task->num_file_descriptors--;
        task->first_free_fdnum = fd;
    }

    void syscall_read(int fd, void *buf, usize count) {
#if SYSCALL_TRACE
        klib::printf("read(%d, %#lX, %ld)\n", fd, (uptr)buf, count);
#endif
        auto *task = (sched::Task*)cpu::read_gs_base();
        if (fd >= (int)task->file_descriptors.size() || fd < 0) return;
        if (fd == 0) {
            ps2::kbd::read(buf, count);
            return;
        }
        FileDescriptor *descriptor = task->file_descriptors[fd];
        if (descriptor == nullptr) return;
        fs::vfs::FileSystem *fs = descriptor->node->fs;
        fs->read(fs, descriptor->node, buf, count, descriptor->cursor);
        descriptor->cursor += count;
    }

    void syscall_pread(int fd, void *buf, usize count, usize offset) {
#if SYSCALL_TRACE
        klib::printf("pread(%d, %#lX, %ld, %ld)\n", fd, (uptr)buf, count, offset);
#endif
        auto *task = (sched::Task*)cpu::read_gs_base();
        if (fd >= (int)task->file_descriptors.size() || fd < 0) return;
        FileDescriptor *descriptor = task->file_descriptors[fd];
        if (descriptor == nullptr) return;
        fs::vfs::FileSystem *fs = descriptor->node->fs;
        fs->read(fs, descriptor->node, buf, count, offset);
    }

    void syscall_write(int fd, const void *buf, usize count) {
#if SYSCALL_TRACE
        klib::printf("write(%d, %#lX, %ld)\n", fd, (uptr)buf, count);
#endif
        auto *task = (sched::Task*)cpu::read_gs_base();
        if (fd >= (int)task->file_descriptors.size() || fd < 0) return;
        if (fd == 1) {
            klib::printf("%.*s", (int)count, (char*)buf);
            return;
        }
        FileDescriptor *descriptor = task->file_descriptors[fd];
        if (descriptor == nullptr) return;
        fs::vfs::FileSystem *fs = descriptor->node->fs;
        fs->write(fs, descriptor->node, buf, count, descriptor->cursor);
        descriptor->cursor += count;
    }

    void syscall_pwrite(int fd, const void *buf, usize count, usize offset) {
#if SYSCALL_TRACE
        klib::printf("pwrite(%d, %#lX, %ld, %ld)\n", fd, (uptr)buf, count, offset);
#endif
        auto *task = (sched::Task*)cpu::read_gs_base();
        if (fd >= (int)task->file_descriptors.size() || fd < 0) return;
        FileDescriptor *descriptor = task->file_descriptors[fd];
        if (descriptor == nullptr) return;
        fs::vfs::FileSystem *fs = descriptor->node->fs;
        fs->write(fs, descriptor->node, buf, count, offset);
    }

    void syscall_seek(int fd, isize offset) {
#if SYSCALL_TRACE
        klib::printf("seek(%d, %ld)\n", fd, offset);
#endif
        auto *task = (sched::Task*)cpu::read_gs_base();
        if (fd >= (int)task->file_descriptors.size() || fd < 0) return;
        FileDescriptor *descriptor = task->file_descriptors[fd];
        if (descriptor == nullptr) return;
        descriptor->cursor = offset;
    }

    isize syscall_getcwd(char *buf, usize size) {
#if SYSCALL_TRACE
        klib::printf("getcwd(%#lX, %ld)\n", (uptr)buf, size);
#endif
        auto *task = (sched::Task*)cpu::read_gs_base();
        klib::Vector<const char*> parent_names;
        Node *current = task->cwd;
        while (true) {
            parent_names.push_back(current->name);
            if (current->parent == nullptr || current->parent->parent == nullptr) // exclude root directory to prevent double slash
                break;
            current = current->parent;
        }
        usize buf_index = 0;
        for (int n = parent_names.size() - 1; n >= 0; n--) {
            buf[buf_index++] = '/';
            if (buf_index >= size)
                return -ERANGE;
            const char *name = parent_names[n];
            while (*name) {
                buf[buf_index++] = *name++;
                if (buf_index >= size)
                    return -ERANGE;
            }
        }
        return 0;
    }

    isize syscall_chdir(const char *path) {
#if SYSCALL_TRACE
        klib::printf("chdir(\"%s\")\n", path);
#endif
        auto *task = (sched::Task*)cpu::read_gs_base();
        DirectoryNode *starting_point = nullptr;
        if (path[0] != '/') // path is relative
            starting_point = task->cwd;
        auto result = path_to_node(path, starting_point);
        if (result.basename != nullptr)
            delete[] result.basename;
        if (result.target == nullptr)
            return -ENOENT;
        if (result.target->type != Node::Type::DIRECTORY)
            return -ENOTDIR;
        task->cwd = (DirectoryNode*)result.target;
        return 0;
    }
}
