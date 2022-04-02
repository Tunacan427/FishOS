#include <mem/allocator.hpp>
#include <kstd/cstring.hpp>
#include <kstd/lock.hpp>
#include <kstd/cstdlib.hpp>
#include <kstd/cstdio.hpp>

static volatile kstd::Spinlock alloc_lock;

namespace mem {
    BuddyAlloc::Block* BuddyAlloc::Block::split_until(usize size) {
        if (size == 0) return nullptr;
        auto block = this;
        auto current_actual_size = size_required(block->size);
        while (size < current_actual_size) {
            current_actual_size /= 2;
            block->size = current_actual_size;
            //kstd::printf("Block %#lX size %#lX split into %#lX & %#lX at size %#lX, requested %#lX\n", (uptr)block, current_actual_size * 2, (uptr)block, (uptr)block->next(), current_actual_size, size);
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
        //kstd::printf("Trying to find best block with size %#lX\n", size);
        Block *best = nullptr;
        Block *block = this->head;
        Block *buddy = block->next();
        
        if (buddy == this->tail && block->is_free) {
            return block->split_until(size);
        }

        while (block < this->tail && buddy < this->tail) { // make sure the buddies are within the range
            // if both buddies are free, coalesce them together
            if (block->is_free && buddy->is_free && block->size == buddy->size) {
                //kstd::printf("Coalescing %#lX & %#lX at size %#lX into block %#lX size %#lX\n", (uptr)block, (uptr)buddy, block->size, (uptr)block, block->size * 2);
                block->size *= 2;
                if (size <= block->size && (best == nullptr || block->size <= best->size))
                    best = block;
                
                block = buddy->next();
                if (block < this->tail)
                    buddy = block->next(); // delay the buddy block for the next iteration
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
            } else {
                // buddy was split into smaller blocks
                block = buddy;
                buddy = buddy->next();
            }
        }
        
        if (best) {
            //kstd::printf("Found best block with size %#lX\n", best->size);
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
                    kstd::printf("Coalescing %#lX & %#lX at size %#lX into block %#lX size %#lX\n", (uptr)block, (uptr)buddy, block->size, (uptr)block, block->size * 2);
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
        alloc_lock.lock();
        void *result = nullptr;

        if (size != 0) {
            usize actual_size = size_required(size + sizeof(BuddyAlloc::Block));

            auto found = this->find_best(actual_size);
            if (!found) {
                this->coalescence();
                found = this->find_best(actual_size);
            }

            if (found) {
                found->size = size + sizeof(BuddyAlloc::Block);
                found->is_free = false;
                result = found->data();
                goto end;
            }
        }

    end:
        alloc_lock.unlock();
        return result;
    }

    void* BuddyAlloc::realloc(void *ptr, usize size) {
        alloc_lock.lock();
        void *result = nullptr;

        if (uptr old_data = (uptr)ptr) {
            if ((uptr)this->head > old_data) goto end;
            if ((uptr)this->tail <= old_data) goto end;
            
            auto old_block = (BuddyAlloc::Block*)(old_data - sizeof(BuddyAlloc::Block));
            usize old_size = old_block->size;
            usize actual_new_size = size_required(size);

            if (size_required(old_size) == actual_new_size) {
                old_block->size = size + sizeof(BuddyAlloc::Block);
                result = old_block;
                goto end;
            }
            
            auto found = this->find_best(actual_new_size);
            if (!found) {
                this->coalescence();
                found = this->find_best(actual_new_size);
            }
            
            if (found) {
                found->size = size + sizeof(BuddyAlloc::Block);
                found->is_free = false;
                result = found->data();
            } else goto end;

            kstd::memcpy(result, ptr, old_size - sizeof(BuddyAlloc::Block));
            old_block->is_free = true;
        }

    end:
        alloc_lock.unlock();
        return result;
    }

    void BuddyAlloc::free(void *ptr) {
        alloc_lock.lock();

        if (uptr data = (uptr)ptr) {
            if ((uptr)this->head > data) goto end;
            if ((uptr)this->tail <= data) goto end;
            
            auto block = (BuddyAlloc::Block*)(data - sizeof(BuddyAlloc::Block));
            block->size = size_required(block->size);
            block->is_free = true;
        }

    end:
        alloc_lock.unlock();
    }
}
