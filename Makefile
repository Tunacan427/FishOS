KERNEL := fishos.elf
ISO := fishos.iso

CC = x86_64-elf-gcc
CPP = x86_64-elf-g++
NASM = nasm

CFLAGS = -O0 -g3 -pipe -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -fsanitize=undefined
CPPFLAGS = $(CFLAGS)
NASMFLAGS = -felf64 -g
LDFLAGS =

INTERNALLDFLAGS :=          \
	-nostdlib               \
	-static                 \
	-z max-page-size=0x1000 \
	-T linker.ld

INTERNALCFLAGS :=           \
	-Isrc                   \
	-ffreestanding          \
	-fstack-protector       \
	-fno-pie                \
	-fno-pic                \
	-fno-omit-frame-pointer \
	-march=x86-64           \
	-mabi=sysv              \
	-mno-80387              \
	-mno-mmx                \
	-mno-sse                \
	-mno-sse2               \
	-mno-red-zone           \
	-mgeneral-regs-only     \
	-mcmodel=kernel         \
	-MMD

INTERNALCPPFLAGS :=     \
	$(INTERNALCFLAGS)   \
	-std=gnu++20        \
	-fno-exceptions     \
	-fno-rtti           \
	-fno-use-cxa-atexit

CFILES := $(shell find ./src -type f -name '*.c')
OBJ := $(CFILES:./src/%.c=obj/%.o)
CPPFILES := $(shell find ./src -type f -name '*.cpp')
OBJ += $(CPPFILES:./src/%.cpp=obj/%.o)
ASMFILES := $(shell find ./src -type f -name '*.asm')
OBJ += $(ASMFILES:./src/%.asm=obj/%.o)
OBJ += obj/font.o

.PHONY: all clean runbios runuefi installuefi

all: $(ISO)

runbios: $(ISO)
	@echo "[QEMU]"
	@qemu-system-x86_64 -cdrom $(ISO) -m 128M -serial stdio \
		-no-reboot -no-shutdown -M smm=off -s

runuefi: installuefi
	@echo "[QEMU]"
	@qemu-system-x86_64 -cdrom $(ISO) -m 128M -serial stdio \
		-no-reboot -no-shutdown -M smm=off -smp 2 \
		-drive if=pflash,format=raw,readonly=on,file=ovmf/OVMF_CODE.fd \
		-drive if=pflash,format=raw,file=ovmf/OVMF_VARS.fd \
		-net nic,model=e1000 -net user -machine q35 \
		-enable-kvm -cpu host

installuefi: $(ISO)
	@echo [UEFI]
	@mkdir -p ovmf
	@cp /usr/share/ovmf/x64/OVMF_CODE.fd ovmf/
	@cp /usr/share/ovmf/x64/OVMF_VARS.fd ovmf/

$(ISO): $(KERNEL)
	@echo "[ISO] $< | $@"
	@mkdir -p isoroot
	@cp $(KERNEL) limine.cfg limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin isoroot/
	@cp userspace/test/build/test initramfs/bin/
	@tar -c --format=ustar -f isoroot/initramfs.tar -C initramfs .
	@xorriso -as mkisofs -b limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		isoroot -o $(ISO)

# Link rules for the final kernel executable.
$(KERNEL): $(OBJ)
	@echo "[LD] $@"
	@$(CPP) $(OBJ) $(LDFLAGS) $(INTERNALLDFLAGS) -o $@

# Compilation rules for *.c files.
obj/%.o: src/%.c
	@echo "[CC] $< | $@"
	@mkdir -p $(shell dirname $@)
	@$(CC) $(CFLAGS) $(INTERNALCFLAGS) -c $< -o $@

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
	@objcopy -O elf64-x86-64 -B i386 -I binary font.psfu obj/font.o

# Remove object files and the final executable.
clean: cleaniso
	rm -rf $(KERNEL) obj/

cleaniso:
	rm -rf $(ISO) isoroot/
