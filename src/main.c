// main
#include <interrupt/exception_handler.h>
#include <interrupt/interrupt.h>
#include <memory/gdt.h>
#include <memory/memory.h>
#include <memory/pfa.h>
#include <memory/vma.h>
#include <module.h>
#include <terminal/terminal.h>
#include <util/io.h>
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

  scan_pde_for_free(page_directory, true);
  vma_alloc(page_directory, 2 * 1024 * 1024, NULL, 0);
  scan_pde_for_free(page_directory, true);

  // test_software_interrupt();
  // start_module(boot_info);
  test_hardware_interrupt();
}
