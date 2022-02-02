#include <mem/allocator.hpp>
#include <mem/manager.hpp>
#include <kstd/cstring.hpp>
#include <kstd/mutex.hpp>
#include <kstd/cstdlib.hpp>
#include <kstd/cstdio.hpp>

static volatile kstd::Mutex allocator_mutex;

static inline usize align(usize i, usize alignment) {
    return i % alignment ? ((i / alignment) + 1) * alignment : i;
}

static inline usize size_required(usize size, usize alignment) {
    usize actual_size = alignment;
    size += sizeof(mem::BuddyBlock);
    size = align(size, alignment); 
    while (size > actual_size)
        actual_size <<= 1;
    return actual_size;
}

namespace mem {
    BuddyBlock* BuddyBlock::split_until(usize size) {
        if (size == 0) return nullptr;
        auto block = this;
        while (size < block->size) {
            usize s = block->size / 2;
            block->size = s;
            kstd::printf("Block %#lX size %#lX split into %#lX & %#lX at size %#lX\n", (uptr)block, s * 2, (uptr)block, (uptr)block->next(), s);
            block = block->next();
            block->size = s;
            block->is_free = true;
        }
        if (size <= block->size)
            return block;
        return nullptr; // death
    }

    void BuddyAlloc::boot(uptr base, usize size) {
        this->head = (BuddyBlock*)base;
        this->head->size = size;
        this->head->is_free = true;
        this->tail = this->head->next();
    }

    BuddyAlloc* BuddyAlloc::get() {
        static BuddyAlloc alloc;
        return &alloc;
    }

    BuddyBlock* BuddyAlloc::find_best(usize size) {
        kstd::printf("Trying to find best block with size %#lX\n", size);
        BuddyBlock *best = nullptr;
        BuddyBlock *block = this->head;
        BuddyBlock *buddy = block->next();
        
        if (buddy == this->tail && block->is_free) {
            return block->split_until(size);
        }

        while (block < this->tail && buddy < this->tail) { // make sure the buddies are within the range
            // if both buddies are free, coalesce them together
            if (block->is_free && buddy->is_free && block->size == buddy->size) {
                kstd::printf("Coalescing %#lX & %#lX at size %#lX into block %#lX size %#lX\n", (uptr)block, (uptr)buddy, block->size, (uptr)block, block->size * 2);
                if (block->size == 0) {
                    kstd::printf("death\n");
                    asm("hlt");
                }
                block->size *= 2;
                if (size <= block->size && (best == nullptr || block->size <= best->size))
                    best = block;
                
                block = buddy->next();
                if (block < this->tail)
                    buddy = block->next(); // delay the buddy block for the next iteration
                continue;
            }
                    
            if (block->is_free && size <= block->size && (best == nullptr || block->size <= best->size))
                best = block;
            
            if (buddy->is_free && size <= buddy->size && (best == nullptr || buddy->size < best->size)) 
                best = buddy;
            
            if (block->size <= buddy->size) {
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
            kstd::printf("Found best block with size %#lX\n", best->size);
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
                } else if (block->size < buddy->size) {
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
        allocator_mutex.lock();

        if (size != 0) {
            usize actual_size = size_required(size, this->alignment);

            auto found = this->find_best(actual_size);
            if (!found) {
                this->coalescence();
                found = this->find_best(actual_size);
            }

            if (found) {
                found->is_free = false;
                allocator_mutex.unlock();
                return found->data();
            }
        }

        allocator_mutex.unlock();
        return nullptr;
    }

    // TODO: maybe implement properly
    void* BuddyAlloc::realloc(void *ptr, usize size) {
        allocator_mutex.lock();
        void *result = nullptr;

        if (uptr old_data = (uptr)ptr) {
            if ((uptr)this->head > old_data) goto end;
            if ((uptr)this->tail <= old_data) goto end;
            
            BuddyBlock *old_block = (BuddyBlock*)(old_data - sizeof(BuddyBlock));
            usize old_size = old_block->size;
            usize actual_new_size = size_required(size, this->alignment);
            
            auto found = this->find_best(actual_new_size);
            if (!found) {
                this->coalescence();
                found = this->find_best(actual_new_size);
            }
            
            if (found) {
                found->is_free = false;
                result = found->data();
            } else goto end;

            kstd::memcpy(result, ptr, old_size - sizeof(BuddyBlock));
            old_block->is_free = true;
        }

    end:
        allocator_mutex.unlock();
        return result;
    }

    void BuddyAlloc::free(void *ptr) {
        allocator_mutex.lock();

        if (uptr data = (uptr)ptr) {
            if ((uptr)this->head > data) goto end;
            if ((uptr)this->tail <= data) goto end;
            
            BuddyBlock *block = (BuddyBlock*)(data - sizeof(BuddyBlock));
            block->is_free = true;
        }

    end:
        allocator_mutex.unlock();
    }
}
