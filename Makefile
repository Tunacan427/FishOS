KERNEL := fishos.elf
ISO := fishos.iso

CPP = x86_64-elf-g++
NASM = nasm

CPPFLAGS = -O0 -pipe -g
NASMFLAGS = -felf64 -g

INTERNALLDFLAGS :=         \
	-Tlinker.ld            \
	-nostdlib              \
	-zmax-page-size=0x1000 \
	-static

INTERNALCPPFLAGS :=      \
	-Isrc                \
	-std=gnu++20         \
	-ffreestanding       \
	-fno-exceptions      \
	-fno-rtti            \
	-fno-stack-protector \
	-fno-pic             \
	-mno-80387           \
	-mno-mmx             \
	-mno-3dnow           \
	-mno-sse             \
	-mno-sse2            \
	-mno-red-zone		 \
    -mcmodel=kernel

CPPFILES := $(shell find ./src -type f -name '*.cpp')
OBJ := $(CPPFILES:./src/%.cpp=obj/%.o)
ASMFILES := $(shell find ./src -type f -name '*.asm')
OBJ += $(ASMFILES:./src/%.asm=obj/%.o)
# OBJ += obj/font.o

.PHONY: all clean run

all: $(ISO)

run: $(ISO)
	@echo "[QEMU]"
	@qemu-system-x86_64 -cdrom $(ISO) -m 128M -serial stdio \
		-drive id=disk,file=disk.img,if=virtio,format=raw -s \
		-no-reboot -no-shutdown -M smm=off -smp 2

$(ISO): $(KERNEL)
	@echo "[ISO] $< | $@"
	@mkdir -p isoroot
	@cp $(KERNEL) limine.cfg limine/limine.sys limine/limine-cd.bin limine/limine-eltorito-efi.bin isoroot/
	@xorriso -as mkisofs -b limine-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table \
        --efi-boot limine-eltorito-efi.bin -efi-boot-part --efi-boot-image --protective-msdos-label \
        isoroot -o $(ISO)

# Link rules for the final kernel executable.
$(KERNEL): $(OBJ)
	@echo "[LD]  $< | $@"
	@$(CPP) $(OBJ) -o $@ $(INTERNALLDFLAGS)

# Compilation rules for *.cpp files.
obj/%.o: src/%.cpp
	@echo "[CPP] $< | $@"
	@mkdir -p $(shell dirname $@)
	@$(CPP) $(CPPFLAGS) $(INTERNALCPPFLAGS) -c $< -o $@

# Compilation rules for *.asm files.
obj/%.o: src/%.asm
	@echo "[ASM] $< | $@"
	@mkdir -p $(shell dirname $@)
	@$(NASM) $(NASMFLAGS) -o $@ $<

obj/font.o:
	objcopy -O elf64-x86-64 -B i386 -I binary font.psfu obj/font.o

# Remove object files and the final executable.
clean:
	rm -rf $(KERNEL) obj/ $(ISO)
