#pragma once

#include <klib/types.hpp>
#include <mem/vmm.hpp>

// EH: ELF header
// PH: Program header
// SH: Section header

// Segment types
#define PT_NULL 0 // Unused
#define PT_LOAD 1 // Loadable
#define PT_DYNAMIC 2 // Dynamic linking info
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5 // Reserved
#define PT_PHDR 6 // Header table

// Segment flags
#define PF_X (1 << 0) // Executable
#define PF_W (1 << 1) // Writable
#define PF_R (1 << 2) // Readable

namespace elf {
    struct [[gnu::packed]] Header {
        u8  identifier[16]; // magic number and other info
        u16 type;
        u16 arch;
        u32 version;
        u64 entry_addr;
        u64 ph_table_offset; // offset in file
        u64 sh_table_offset; // offset in file
        u32 flags; // arch specific flags
        u16 eh_size;
        u16 ph_entry_size;
        u16 ph_count;
        u16 sh_entry_size;
        u16 sh_count;
        u16 sh_str_table_index;
    };

    struct [[gnu::packed]] ProgramHeader {
        u32 type;
        u32 flags;
        u64 offset; // file offset
        u64 virt_addr;
        u64 phy_addr;
        u64 file_size; // segment size in file
        u64 mem_size; // segment size in memory
        u64 alignment;
    };

    uptr load(mem::vmm::Pagemap *pagemap, uptr file_addr);
}
