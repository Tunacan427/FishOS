CC = x86_64-elf-gcc
CPP = x86_64-elf-g++

CFLAGS = -O0 -g3 -pipe -Wall -Wextra
CPPFLAGS = $(CFLAGS)
LDFLAGS =

BIN = build/test

INTERNALLDFLAGS :=          \
	-nostdlib               \
	-static                 \
	-z max-page-size=0x1000 \
	-T linker.ld

INTERNALCFLAGS :=           \
	-ffreestanding          \
	-fno-pie                \
	-fno-pic                \
	-march=x86-64           \
	-mabi=sysv              \
	-mno-80387              \
	-mno-mmx                \
	-mno-sse                \
	-mno-sse2               \
	-mno-red-zone

INTERNALCPPFLAGS :=         \
	$(INTERNALCFLAGS)       \
	-std=gnu++20            \
	-fno-exceptions         \
	-fno-rtti               \
	-fno-use-cxa-atexit

CPPFILES := $(shell find ./src -type f -name '*.cpp')
OBJ += $(CPPFILES:./src/%.cpp=build/%.o)

.PHONY: all clean

all: $(BIN)

# Link rules for the final executable.
$(BIN): $(OBJ)
	@echo "[LD] $@"
	@$(CPP) $(OBJ) $(LDFLAGS) $(INTERNALLDFLAGS) -o $@

# Compilation rules for *.c files.
build/%.o: src/%.c
	@echo "[CC] $< | $@"
	@mkdir -p $(shell dirname $@)
	@$(CC) $(CFLAGS) $(INTERNALCFLAGS) -c $< -o $@

# Compilation rules for *.cpp files.
build/%.o: src/%.cpp
	@echo "[CPP] $< | $@"
	@mkdir -p $(shell dirname $@)
	@$(CPP) $(CPPFLAGS) $(INTERNALCPPFLAGS) -c $< -o $@

# Remove object files and the final executable.
clean:
	rm -rf $(BIN) build/
