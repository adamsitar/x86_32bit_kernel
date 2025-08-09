// main
#include "vmm.h"
#include <exception_handler.h>
#include <gdt.h>
#include <interrupt.h>
#include <io.h>
#include <memory.h>
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

  // PDE 0 and PDE 768 are filled in boot
  // scan_pde_for_free(page_directory, true);
  // alloc_new_pt(page_directory, 767); // 768 causes a page fault
  printf("virtual addr: %x\n", vmm_alloc(page_directory, 2000, 0xC020000, 0));
  char *a = (char *)0xC020000;
  *a = 20;
  printf("char is: %u", *a);

  // test_software_interrupt();
  // start_module(boot_info);
  test_hardware_interrupt();
}
