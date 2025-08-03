file bin/os.bin
target remote localhost:1234
set architecture i386
set breakpoint pending on

break _start
continue

break kernel_main
