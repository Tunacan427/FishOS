#include <userland/elf.hpp>
#include <klib/cstring.hpp>
#include <klib/cstdio.hpp>
#include <mem/vmm.hpp>
#include <mem/pmm.hpp>
#include <panic.hpp>

namespace userland::elf {
    uptr load(mem::vmm::Pagemap *pagemap, fs::vfs::FileNode *file) {
        Header header {};
        file->fs->read(file->fs, file, &header, sizeof(Header), 0);

        if (klib::memcmp(header.identifier, "\177ELF", 4))
            panic("Not an ELF file (incorrect magic number)");

        // check:
        // - class must be 64-bit
        // - data must be 2's complement, little endian
        // - ABI must be System V
        // - architecture must be x86_64
        if (header.identifier[4] != 2 || header.identifier[5] != 1 || header.identifier[7] != 0 || header.arch != 62)
            panic("Unsupported ELF file");

        for (usize i = 0; i < header.ph_count; i++) {
            ProgramHeader ph {};
            file->fs->read(file->fs, file, &ph, sizeof(ProgramHeader), header.ph_table_offset + i * header.ph_entry_size);

            switch (ph.type) {
            case PT_LOAD: {
                u64 page_flags = PAGE_PRESENT | PAGE_USER;

                if (ph.flags & PF_W)
                    page_flags |= PAGE_WRITABLE;
                
                if (!(ph.flags & PF_X))
                    page_flags |= PAGE_NO_EXECUTE;
                
                usize misalign = ph.virt_addr & 0xFFF;
                usize mem_page_count = (ph.mem_size + misalign + 0x1000 - 1) / 0x1000;
                usize file_page_count = (ph.file_size + misalign + 0x1000 - 1) / 0x1000;
                
                for (usize i = 0; i < mem_page_count; i++) {
                    uptr page_phy = mem::pmm::alloc_pages(1);
                    pagemap->map_page(page_phy, ph.virt_addr + (i * 0x1000), page_flags);
                    uptr dst = page_phy + mem::vmm::get_hhdm();
                    klib::memset((void*)dst, 0, 0x1000); // TODO: optimize this
                    if (i < file_page_count) {
                        uptr offset = ph.offset + (i * 0x1000);
                        uptr size = 0x1000;
                        if (i == 0) {
                            dst += misalign;
                            offset += misalign;
                            size -= misalign;
                        } else if (i == file_page_count - 1) {
                            size = (ph.file_size + misalign) % 0x1000;
                        }
                        file->fs->read(file->fs, file, (void*)dst, size, offset);
                    }
                }
                
                break;
            }
            default:
                klib::printf("ELF Loader: Unrecognized program header type: %#X\n", ph.type);
            }
        }
        
        return header.entry_addr;
    }
}
