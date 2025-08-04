#pragma once
#include <io.h>
#include <keyboard.h>
#include <pic.h>
#include <stdbool.h>
#include <stdint.h>
#include <terminal.h>

// IDT Register structure - points the CPU to the IDT's base address and size
// (limit).
typedef struct {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed)) idtr_t;
static idtr_t idtr;

// 32-bit IDT entry - each interrupt vector has one of these
typedef struct {
  uint16_t isr_low;   // Bits 0-15 of handler address
  uint16_t kernel_cs; // GDT selector (usually 0x08 for kernel code)
  uint8_t reserved;   // Must be 0
  uint8_t attributes; // Gate type, DPL, present bit
  uint16_t isr_high;  // Bits 16-31 of handler address
} __attribute__((packed)) idt_entry_t;

// attributes breakdown:
// Bit 7: Present (P) - must be 1 for valid entries
// Bit 6-5: Descriptor Privilege Level (DPL) - 0 for kernel, 3 for user
// Bit 4: Storage Segment (S) - must be 0 for interrupt gates
// Bit 3-0: Gate Type:
//   - 0x5 = Task Gate (32-bit)
//   - 0xE = Interrupt Gate (32-bit)
//   - 0xF = Trap Gate (32-bit)

idt_entry_t idt[256] = {};

// populates an IDT entry with the address of an interrupt handler
void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags) {
  idt_entry_t *descriptor = &idt[vector];

  descriptor->isr_low = (uint32_t)isr & 0xFFFF;
  descriptor->kernel_cs = 0x08; // this value can be whatever offset your kernel
                                // code selector is in your GDT
  descriptor->attributes = flags;
  descriptor->isr_high = (uint32_t)isr >> 16;
  descriptor->reserved = 0;
}

#define IDT_MAX_DESCRIPTORS 256
static bool vectors[IDT_MAX_DESCRIPTORS];
extern void *isr_stub_table[];

void idt_init();

// Interrupt frame structure matching the stack layout
typedef struct {
  // Segment register (pushed by our assembly)
  uint32_t ds;
  // General purpose registers (pushed by pushad in reverse order)
  uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
  // Our data (pushed by our assembly)
  uint32_t interrupt_num;
  uint32_t error_code;
  // CPU-pushed data (pushed automatically during interrupt)
  uint32_t eip;    // Instruction pointer where interrupt occurred
  uint32_t cs;     // Code segment
  uint32_t eflags; // CPU flags register
  // Only present if interrupt occurred while in user mode (different privilege)
  // uint32_t user_esp;   // User stack pointer
  // uint32_t user_ss;    // User stack segment
} __attribute__((packed)) interrupt_frame_t;

static inline bool are_interrupts_enabled() {
  unsigned long flags;
  asm volatile("pushf\n\t"
               "pop %0"
               : "=g"(flags));
  return flags & (1 << 9);
}

// Function to trigger a software interrupt (vector 3) for testing
void test_software_interrupt(void) {
  // Inline assembly to trigger 'int 3' (breakpoint exception)
  __asm__ volatile("int $3" // Triggers vector 3 in the IDT
  );
}

// will recieve interrupts from QEMU
// (IRQ 0 → vector 32) should fire every ~18ms if enabled)
void test_hardware_interrupt(void) {
  while (1) {
    // Infinite loop to keep kernel running and allow interrupts
    __asm__ volatile("hlt"); // Low-power wait for interrupts
  }
}

void exception_handler(interrupt_frame_t *frame);

void idt_init() {
  // Set up IDT register
  idtr.base = (uintptr_t)&idt[0];
  idtr.limit = (uint16_t)sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1;

  // Install exception handlers (0-31)
  for (uint8_t vector = 0; vector < 32; vector++) {
    idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
    vectors[vector] = true;
  }

  // Remap PIC: exceptions use 0-31, so put hardware IRQs at 32-47
  pic_remap(32, 40);
  // Install IRQ handlers (32-47) - table indices 32-47
  for (uint8_t vector = 0; vector < 16; vector++) {
    idt_set_descriptor(32 + vector, isr_stub_table[32 + vector], 0x8E);
    vectors[32 + vector] = true; // Optional: Track these too
  }

  // Load new IDT
  __asm__ volatile("lidt %0" : : "m"(idtr));

  // Enable specific IRQs we want
  irq_clear_mask(1); // Enable keyboard (IRQ 1)
  // irq_clear_mask(0); // Enable timer (IRQ 0)

  // Enable interrupts via the interrupt flag
  __asm__ volatile("sti");

  if (are_interrupts_enabled()) {
    printf("interrupts enabled!\n");
  } else {
    printf("interrupts disabled!\n");
  }
}
