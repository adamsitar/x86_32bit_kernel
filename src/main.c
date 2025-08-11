// main
#include "vma.h"
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
  setup_recursive_pd();

  // PDE 0 and PDE 768 are filled in boot
  // scan_pde_for_free(page_directory, true);
  // alloc_new_pt(page_directory, 767); // 768 causes a page fault
  uint32_t virt_addr = vma_alloc(page_directory, 3 * 1024 * 1024, 0xC020000, 0);
  printf("virtual addr: 0x%x\n", virt_addr);
  scan_pde_for_free(page_directory, true);
  vma_free(page_directory, virt_addr, 5 * 1024 * 1024);
  scan_pde_for_free(page_directory, true);

  // test_software_interrupt();
  // start_module(boot_info);
  test_hardware_interrupt();
}
