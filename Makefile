# Makefile for building a bare-metal kernel

# Directories
SRC_DIR := src
ASM_DIR := $(SRC_DIR)/asm
BIN_DIR := bin
ISO_DIR := $(BIN_DIR)/isodir
PROGRAM_DIR := $(ASM_DIR)/programs
PROGRAM_BIN_DIR := $(BIN_DIR)/programs

# Compilers and tools
NASM := nasm
CC := i686-elf-gcc
LD := ld
GRUB_MKRESCUE := grub-mkrescue
QEMU := qemu-system-i386

# Flags
NASM_FLAGS := -felf32 -I$(ASM_DIR) -I$(SRC_DIR)
CFLAGS := -std=gnu99 -ffreestanding -g -Wall -Wextra -I$(SRC_DIR) -I$(ASM_DIR)
LDFLAGS := -m elf_i386 -T linker.ld
QEMU_FLAGS := -no-reboot -no-shutdown -d int,guest_errors,invalid_mem

# Source files (using wildcard to automatically find all .asm and .c files)
# This uses Make's wildcard function to glob files dynamically.
ASM_SOURCES := $(wildcard $(ASM_DIR)/*.nasm)
C_SOURCES := $(wildcard $(SRC_DIR)/*.c)

# Find all program source files
PROGRAM_SOURCES := $(wildcard $(PROGRAM_DIR)/*.nasm)
# Convert to .bin targets
PROGRAM_BINS := $(patsubst $(PROGRAM_DIR)/%.nasm,$(BIN_DIR)/programs/%.bin,$(PROGRAM_SOURCES))

# Object files (using patsubst to transform source paths to bin/ paths)
# patsubst is a substitution function: $(patsubst pattern, replacement, text)
ASM_OBJECTS := $(patsubst $(ASM_DIR)/%.nasm, $(BIN_DIR)/%.o, $(ASM_SOURCES))
C_OBJECTS := $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.o, $(C_SOURCES))

# All objects combined
OBJECTS := $(ASM_OBJECTS) $(C_OBJECTS)

all: clean run

# Rule to link all objects into bin/os.bin
# $@ is an automatic variable for the target name.
# $^ is an automatic variable for all prerequisites (no duplicates).
$(BIN_DIR)/os.bin: $(OBJECTS) | $(BIN_DIR)
	echo $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

# Pattern rule for compiling ASM files
# % is a wildcard in pattern rules; $< is the first prerequisite (source file),
# $@ is the target (object file).
$(BIN_DIR)/%.o: $(ASM_DIR)/%.nasm | $(BIN_DIR)
	$(NASM) $(NASM_FLAGS) $< -o $@

$(PROGRAM_BIN_DIR)/%.bin: $(PROGRAM_DIR)/%.nasm | $(PROGRAM_BIN_DIR)
	$(NASM) -f bin $< -o $@

# Pattern rule for compiling C files
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to create the ISO
# This depends on bin/os.bin and grub.cfg (assumed to be in root).
# We use mkdir -p to create the directory structure if it doesn't exist.
iso: $(BIN_DIR)/os.bin grub.cfg $(PROGRAM_BINS)
	mkdir -p $(ISO_DIR)/boot/grub
	mkdir -p $(ISO_DIR)/modules
	cp $(BIN_DIR)/os.bin $(ISO_DIR)/boot/os.bin
	cp $(PROGRAM_BIN_DIR)/program.bin $(ISO_DIR)/modules/program.bin
	cp grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o $(BIN_DIR)/os.iso $(ISO_DIR)

# Rule to run QEMU
run: iso
	# $(QEMU) -cdrom $(BIN_DIR)/os.iso -d int -s -S -no-shutdown -no-reboot
	# $(QEMU) -cdrom $(BIN_DIR)/os.iso
	#
	$(QEMU) $(QEMU_FLAGS) -kernel $(BIN_DIR)/os.bin -s -S
	# $(QEMU) $(QEMU_FLAGS) -kernel $(BIN_DIR)/os.bin 
	

# Create bin/ directory if it doesn't exist (order-only prerequisite, using |)
# Order-only prerequisites (with |) are not checked for timestamps, just ensured to exist.
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(PROGRAM_BIN_DIR): 
	mkdir -p $(PROGRAM_BIN_DIR)

# Clean up build artifacts
clean:
	rm -rf $(BIN_DIR)/*

# Phony targets: these are not files, so always run them regardless of timestamps
.PHONY: all iso run clean

# backup of the old build.sh script
# nasm -felf32 boot.asm -o bin/boot.o
# nasm -felf32 io.asm -o bin/io.o
# i686-elf-gcc -c kernel.c -o bin/kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
# ld -m elf_i386 -T linker.ld -o bin/os.bin bin/boot.o bin/kernel.o bin/io.o
#
# # create the folder structure and the iso
# mkdir -p bin/isodir/boot/grub
# cp bin/os.bin bin/isodir/boot/os.bin
# cp grub.cfg bin/isodir/boot/grub/grub.cfg
# grub-mkrescue -o bin/os.iso bin/isodir
#
# qemu-system-i386 -cdrom bin/os.iso
