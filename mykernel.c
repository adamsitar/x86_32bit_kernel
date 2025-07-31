// kernel.c
// Place the Multiboot header at the top
__attribute__((section(".multiboot"), used))
const unsigned int multiboot_header[] = {0x1BADB002,
                                         0x00, // minimal flags
                                         -(0x1BADB002 + 0x00)};

void kernel_main(void) {
  // running in kernel
  for (;;) {
  } // Halt loop
}
