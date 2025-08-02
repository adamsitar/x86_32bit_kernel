// main
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

  printf("hello!\n");
  // fb_move_cursor(7);
  if (are_interrupts_enabled()) {
    printf("interrupts enabled!\n");
  } else {
    printf("interrupts disabled!\n");
  }
}
