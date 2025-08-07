// main
#include <exception_handler.h>
#include <gdt.h>
#include <interrupt.h>
#include <io.h>
#include <multiboot.h>
#include <pfa.h>
#include <terminal.h>
// standard
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Typedef for a function pointer that takes no arguments and returns void
// This matches the book's suggestion: it's how we "cast" the module's address
// to something callable
typedef void (*call_module_t)(void);

// Halt the CPU in an infinite loop (useful for error handling or if the module
// returns)
void halt() {
  asm volatile("cli; hlt"); // Disable interrupts and halt (similar to your
                            // assembly hang loop)
  while (1) {
  } // Infinite loop to ensure we never proceed
}

void clear_identity_mapping() {
  extern uint32_t page_directory[1024];
  extern uint32_t page_table[1024];

  // Step 1: Clear PDE 0 (identity mapping).
  // page_directory is at high virtual address.
  page_directory[0] = 0; // Set to 0 (not present, no flags).

  // Step 2: Invalidate TLB.
  // Option A: Full TLB flush by reloading CR3 (simple).
  unsigned int cr3_val;
  asm volatile("mov %%cr3, %0" : "=r"(cr3_val));  // Read CR3
  asm volatile("mov %0, %%cr3" : : "r"(cr3_val)); // Write back to flush TLB.
}

void test_identity_clear() {
  // (Optional) Step 3: Verify - this should cause a page fault.
  volatile unsigned int *test = (unsigned int *)0x00000000;
  *test = 0xDEADBEEF; // Will fault if identity mapping is removed.
}

void start_module(multiboot_info_t *mbi) {
  // Step 1: Check if module info is available (flags bit 3 must be set)
  if (!(mbi->flags & (1 << 3))) {
    // No modules loaded? Handle the error (e.g., halt or log)
    // For now, we'll just halt
    halt();
  }

  printf("module count: %u\n", mbi->mods_count);
  // Step 2: Check if at least one module was loaded
  if (mbi->mods_count == 0) {
    // No modules? Halt or handle error
    halt();
  }

  // Step 3: Get the address of the first module's structure
  // mods_addr points to an array of multiboot_module_t structs
  multiboot_module_t *mod = (multiboot_module_t *)mbi->mods_addr;

  // Step 4: Cast the module's start address to a callable function pointer
  // This assumes the module's code begins at mod->mod_start and is executable
  call_module_t start_program = (call_module_t)mod->mod_start;

  // Step 5: Call (jump to) the module's code
  start_program();
}

void kernel_main(uint32_t magic, multiboot_info_t *boot_info) {
  init_terminal();
  init_gdt();
  init_idt();
  init_pfa(boot_info); // Call our initializer
  alloc_new_pt(769);

  // test_software_interrupt();
  // start_module(mbi);
  test_hardware_interrupt();
}
