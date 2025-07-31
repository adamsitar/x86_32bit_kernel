nasm -f elf32 -o bin/loader.o loader.s
ld -T link.ld -melf_i386 loader.o -o bin/kernel.elf
grub-mkrescue -o bin/os.iso iso
bochs -f bochsrc.txt -q
