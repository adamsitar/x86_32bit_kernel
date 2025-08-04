#include <interrupt.h>

enum IRQ { TIMER = 32, KEYBOARD = 33 };

// Helper to read CR2 (faulting address) - inline asm
static inline uintptr_t read_cr2() {
  uintptr_t cr2;
  asm volatile("mov %%cr2, %0" : "=r"(cr2));
  return cr2;
}

void exception_handler(interrupt_frame_t *frame) {
  if (frame->interrupt_num != TIMER) {
    printf("Interrupt received! Vector: %d\n", frame->interrupt_num);
  }

  // is PIC IRQ
  if (frame->interrupt_num >= 32 && frame->interrupt_num <= 47) {

    if (frame->interrupt_num == KEYBOARD) {
      keyboard_handler();
    }
    // TODO Handle other IRQs (e.g., vector 32 for timer)

    // This is a hardware IRQ: Acknowledge the PIC
    pic_acknowledge(frame->interrupt_num);

    // For IRQs, we can return normally (iret will resume execution)
    return;
  } else {
    switch (frame->interrupt_num) {
    case 8: // Double Fault (always fatal - kernel bug or unhandled exception)
      printf("Double Fault (fatal)! Error code: %d\n", frame->error_code);
      printf("  - EIP: %p, CS: %x, EFLAGS: %x\n", frame->eip, frame->cs,
             frame->eflags);
      asm volatile("cli; hlt"); // Halt safely
      break;

    case 14: // Page Fault (can be non-fatal, e.g., for demand paging)
    {
      uintptr_t fault_addr = read_cr2(); // Get faulting virtual address
      printf("Page Fault! Fault address: %p, Error code: %d\n", fault_addr,
             frame->error_code);
      // Decode error code bits for more info
      printf("  - Cause: %s%s%s%s\n",
             (frame->error_code & 1) ? "Page-level protection violation"
                                     : "Non-present page",
             (frame->error_code & 2) ? ", Write" : ", Read",
             (frame->error_code & 4) ? ", User mode" : ", Kernel mode",
             (frame->error_code & 8) ? ", Reserved bit violation" : "");
      printf("  - EIP: %p (instruction that caused fault)\n", frame->eip);
      // TODO: In a real kernel, handle (e.g., allocate page) or kill process
      asm volatile("cli; hlt"); // For now, halt - replace with recovery later
    } break;

    default: // Other exceptions/software interrupts: Treat as fatal
      printf("Fatal exception! Vector: %d, Error code: %d\n",
             frame->interrupt_num, frame->error_code);
      asm volatile("cli; hlt");
      break;
    }
    // This is an exception (or software interrupt): Treat as fatal
    printf("Fatal exception! Error code: %d\n", frame->error_code);
    __asm__ volatile("cli; hlt"); // Disable interrupts and halt
  }
}
