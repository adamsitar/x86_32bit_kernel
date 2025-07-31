nasm -felf32 boot.asm -o bin/boot.o
nasm -felf32 kernel.asm -o bin/kernel.o
ld -m elf_i386 -T linker.ld -o bin/os.bin bin/boot.o bin/kernel.o
qemu-system-i386 -kernel bin/os.bin
grub-mkrescue -o myos.iso isodir
qemu-system-i386 -cdrom myos.iso
