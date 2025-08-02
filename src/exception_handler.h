#include <interrupt.h>

enum IRQ { KEYBOARD = 33 };

void exception_handler(interrupt_frame_t *frame) {
  // if (frame->interrupt_num != 32) {
  //   printf("Interrupt received! Vector: %d\n", frame->interrupt_num);
  // }

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
    // This is an exception (or software interrupt): Treat as fatal
    printf("Fatal exception! Error code: %d\n", frame->error_code);
    __asm__ volatile("cli; hlt"); // Disable interrupts and halt
  }
}
