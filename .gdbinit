file bin/os.bin
target remote localhost:1234
set architecture i386
set breakpoint pending on

# GRUB boot address of kernel
break *0x00100000 
break _start
continue

break kernel_main
