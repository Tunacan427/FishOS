#include <mem/allocator.hpp>
#include <klib/cstring.hpp>
#include <klib/lock.hpp>
#include <klib/cstdio.hpp>

namespace mem {
    static klib::Spinlock alloc_lock;

    BuddyAlloc::Block* BuddyAlloc::Block::split_until(usize size) {
        if (size == 0) return nullptr;
        auto block = this;
        auto current_actual_size = size_required(block->size);
        while (size < current_actual_size) {
            current_actual_size /= 2;
            block->size = current_actual_size;
            // klib::printf("%#lX size %#lX split -> %#lX & %#lX size %#lX, req %#lX\n", (uptr)block, current_actual_size * 2, (uptr)block, (uptr)block->next(), current_actual_size, size);
            block = block->next();
            block->size = current_actual_size;
            block->is_free = true;
        }
        if (size <= current_actual_size)
            return block;
        return nullptr; // death
    }

    void BuddyAlloc::init(uptr base, usize size) {
        this->head = (Block*)base;
        this->head->size = size;
        this->head->is_free = true;
        this->tail = this->head->next();
    }

    BuddyAlloc* BuddyAlloc::get() {
        static BuddyAlloc alloc;
        return &alloc;
    }

    BuddyAlloc::Block* BuddyAlloc::find_best(usize size) {
        // klib::printf("Trying to find best block with size %#lX\n", size);
        Block *best = nullptr;
        Block *block = this->head;
        Block *buddy = block->next();
        // klib::printf("1 block: %#lX, buddy: %#lX\n", (uptr)block, (uptr)buddy);
        
        if (buddy == this->tail && block->is_free) {
            return block->split_until(size);
        }

        while (block < this->tail && buddy < this->tail) { // make sure the buddies are within the range
            // if both buddies are free, coobjalesce them together
            if (block->is_free && buddy->is_free && block->size == buddy->size) {
                // klib::printf("Coalescing %#lX & %#lX at size %#lX into block %#lX size %#lX\n", (uptr)block, (uptr)buddy, block->size, (uptr)block, block->size * 2);
                block->size *= 2;
                if (size <= block->size && (best == nullptr || block->size <= best->size))
                    best = block;
                
                block = buddy->next();
                if (block < this->tail)
                    buddy = block->next(); // delay the buddy block for the next iteration
                // klib::printf("2 block: %#lX, buddy: %#lX\n", (uptr)block, (uptr)buddy);
                continue;
            }
            
            auto actual_best_size = best ? size_required(best->size) : 0;
            if (block->is_free && size <= block->size && (best == nullptr || block->size <= actual_best_size))
                best = block;
            
            if (buddy->is_free && size <= buddy->size && (best == nullptr || buddy->size < actual_best_size)) 
                best = buddy;
            
            if (size_required(block->size) <= size_required(buddy->size)) {
                block = buddy->next();
                if (block < this->tail)
                    buddy = block->next(); // delay the buddy block for the next iteration
                // klib::printf("3 block: %#lX, buddy: %#lX\n", (uptr)block, (uptr)buddy);
            } else {
                // buddy was split into smaller blocks
                block = buddy;
                buddy = buddy->next();
                // klib::printf("4 block: %#lX, buddy: %#lX\n", (uptr)block, (uptr)buddy);
            }
        }
        
        if (best) {
            // klib::printf("Found best block with size %#lX\n", best->size);
            return best->split_until(size);
        }
        return nullptr;
    }

    void BuddyAlloc::coalescence() {
        while (1) { // keep looping until there are no more buddies to coalesce
            auto block = this->head;   
            auto buddy = block->next();
            
            bool no_coalescence = true;
            while (block < this->tail && buddy < this->tail) { // make sure the buddies are within the range
                if (block->is_free && buddy->is_free && block->size == buddy->size) {
                    // coalesce buddies into one
                    // klib::printf("Coalescing %#lX & %#lX at size %#lX into block %#lX size %#lX\n", (uptr)block, (uptr)buddy, block->size, (uptr)block, block->size * 2);
                    block->size *= 2;
                    block = block->next();
                    if (block < this->tail) {
                        buddy = block->next();
                        no_coalescence = false;
                    }
                } else if (size_required(block->size) < size_required(buddy->size)) {
                    // split the buddy block into smaller blocks
                    block = buddy;
                    buddy = buddy->next();
                } else {
                    block = buddy->next();
                    if (block < this->tail) 
                        buddy = block->next(); // leave the buddy block for the next iteration
                }
            }
            
            if (no_coalescence)
                return;
        }
    }

    void* BuddyAlloc::malloc(usize size) {
        klib::LockGuard guard(alloc_lock);

        if (size != 0) {
            usize actual_size = size_required(size + sizeof(Block));

            auto found = this->find_best(actual_size);
            if (!found) {
                this->coalescence();
                found = this->find_best(actual_size);
            }

            if (found) {
                found->size = size + sizeof(Block);
                found->is_free = false;
                return found->data();
            }
        }

        return nullptr;
    }

    void* BuddyAlloc::realloc(void *ptr, usize size) {
        if (ptr == nullptr)
            return malloc(size);

        if (size == 0) {
            free(ptr);
            return nullptr;
        }

        klib::LockGuard guard(alloc_lock);

        if (uptr old_data = (uptr)ptr) {
            if ((uptr)this->head > old_data) return nullptr;
            if ((uptr)this->tail <= old_data) return nullptr;
            
            auto old_block = (Block*)(old_data - sizeof(Block));
            usize old_size = old_block->size;
            usize actual_new_size = size_required(size + sizeof(Block));

            if (size_required(old_size) == actual_new_size) {
                old_block->size = size + sizeof(Block);
                return old_block->data();
            }
            
            auto found = this->find_best(actual_new_size);
            if (!found) {
                this->coalescence();
                found = this->find_best(actual_new_size);
            }
            
            if (found) {
                found->size = size + sizeof(Block);
                found->is_free = false;
                klib::memcpy(found->data(), ptr, old_size - sizeof(Block));
                old_block->is_free = true;
                return found->data();
            }
        }

        return nullptr;
    }

    void BuddyAlloc::free(void *ptr) {
        klib::LockGuard guard(alloc_lock);

        if (uptr data = (uptr)ptr) {
            if ((uptr)this->head > data) return;
            if ((uptr)this->tail <= data) return;
            
            auto block = (Block*)(data - sizeof(Block));
            block->size = size_required(block->size);
            block->is_free = true;
        }
    }
}
