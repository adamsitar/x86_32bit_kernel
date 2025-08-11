#include <interrupt/interrupt.h>

enum IRQ { DOUBLE_FAULT = 8, PAGE_FAULT = 14, TIMER = 32, KEYBOARD = 33 };

// Helper to read CR2 (faulting address) - inline asm
static inline uintptr_t read_cr2() {
  uintptr_t cr2;
  asm volatile("mov %%cr2, %0" : "=r"(cr2));
  return cr2;
}

// can be non-fatal, e.g., for demand paging
void page_fault_handler(interrupt_frame_t *frame) {
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
  // TODO handle (e.g., allocate page) or kill process
  asm volatile("cli; hlt"); // halt - replace with recovery later
}

// always fatal
void double_fault_handler(interrupt_frame_t *frame) {
  printf("Double Fault (fatal)! Error code: %d\n", frame->error_code);
  printf("  - EIP: %p, CS: %x, EFLAGS: %x\n", frame->eip, frame->cs,
         frame->eflags);
  asm volatile("cli; hlt"); // Halt safely
}

void default_handler(interrupt_frame_t *frame) {
  printf("Fatal exception! Vector: %d, Error code: %d\n", frame->interrupt_num,
         frame->error_code);
  asm volatile("cli; hlt");
}

void exception_handler(interrupt_frame_t *frame) {
  if (frame->interrupt_num != TIMER && frame->interrupt_num != KEYBOARD) {
    printf("Interrupt received! Vector: %d\n", frame->interrupt_num);
  }

  // is PIC IRQ
  if (frame->interrupt_num >= 32 && frame->interrupt_num <= 47) {
    if (frame->interrupt_num == KEYBOARD) {
      keyboard_handler();
    }
    // TODO Handle other IRQs (e.g., vector 32 for timer)

    pic_acknowledge(frame->interrupt_num);

    return;
  } else {
    switch (frame->interrupt_num) {
    case DOUBLE_FAULT:
      double_fault_handler(frame);
      break;
    case PAGE_FAULT: {
      page_fault_handler(frame);
    } break;
    default:
      default_handler(frame);
      break;
    }
  }
}
