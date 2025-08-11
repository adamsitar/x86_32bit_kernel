#pragma once
#include <multiboot_gnu.h>
#include <printf.h>
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
