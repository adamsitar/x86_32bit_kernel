// main
#include <exception_handler.h>
#include <gdt.h>
#include <interrupt.h>
#include <io.h>
#include <terminal.h>
// standard
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void kernel_main(void) {
  init_gdt();
  terminal_initialize();
  idt_init();

  // test_software_interrupt();
  test_hardware_interrupt();
}
