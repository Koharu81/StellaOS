# Compiler and tools
CC = gcc
AS = nasm
LD = ld

# Flags
CFLAGS = -std=gnu99 -m32 -ffreestanding -fno-builtin -fno-stack-protector -fno-pie \
         -nostdlib -nostdinc -Wall -Wextra -O2 -Iinclude \
         -mno-sse -mno-sse2 -mno-mmx -mno-80387 -mno-red-zone
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

# Source files
C_SOURCES = $(wildcard kernel/*.c)
ASM_SOURCES = $(wildcard kernel/*.asm)

# Object files
C_OBJECTS = $(C_SOURCES:.c=.o)
ASM_OBJECTS = $(ASM_SOURCES:.asm=.o)
OBJECTS = kernel/boot.o $(filter-out kernel/boot.o, $(ASM_OBJECTS)) $(C_OBJECTS)

# Output
KERNEL = build/stella.bin
ISO = build/StellaOS.iso

# Default target
all: $(ISO)

# Create build directory
build:
	mkdir -p build

# Compile C files
kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble ASM files
kernel/%.o: kernel/%.asm
	$(AS) $(ASFLAGS) $< -o $@

# Link kernel
$(KERNEL): build $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

# Create ISO
$(ISO): $(KERNEL)
	mkdir -p build/iso/boot/grub
	cp $(KERNEL) build/iso/boot/stella.bin
	cp boot/grub/grub.cfg build/iso/boot/grub/grub.cfg
	grub-mkrescue -o $@ build/iso 2>/dev/null

# Clean
clean:
	rm -rf build
	rm -f kernel/*.o

.PHONY: all clean
