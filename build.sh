# TODO improve script and probably write a makefile
nasm -felf32 boot.asm -o bin/boot.o
nasm -felf32 io.asm -o bin/io.o
i686-elf-gcc -c kernel.c -o bin/kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
ld -m elf_i386 -T linker.ld -o bin/os.bin bin/boot.o bin/kernel.o bin/io.o

# create the folder structure and the iso
mkdir -p bin/isodir/boot/grub
cp bin/os.bin bin/isodir/boot/os.bin
cp grub.cfg bin/isodir/boot/grub/grub.cfg
grub-mkrescue -o bin/os.iso bin/isodir

qemu-system-i386 -cdrom bin/os.iso
