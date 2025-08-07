// main
#include <exception_handler.h>
#include <gdt.h>
#include <interrupt.h>
#include <io.h>
#include <pfa.h>
#include <terminal.h>
// standard
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void kernel_main(uint32_t magic, multiboot_info_t *boot_info) {
  terminal_init();
  init_gdt();
  init_idt();
  init_pfa(boot_info); // Call our initializer
  // alloc_new_pt(769);
  // alloc_new_pt(800);
  // alloc_new_pt(801);

  // test_software_interrupt();
  // start_module(boot_info);
  test_hardware_interrupt();
}
