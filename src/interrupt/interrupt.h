#pragma once
#include <interrupt/pic.h>
#include <stdbool.h>
#include <stdint.h>
#include <terminal/keyboard.h>
#include <terminal/terminal.h>
#include <util/io.h>
#include <util/util.h>

#define IDT_MAX_DESCRIPTORS 256

// 32-bit IDT entry - each interrupt vector has one of these
typedef struct {
  uint16_t isr_low;  // Bits 0-15 of handler address
  uint16_t selector; // GDT selector (usually 0x08 for kernel code)
  uint8_t reserved;  // Must be 0
  uint8_t flags;     // Gate type, DPL, present bit
  uint16_t isr_high; // Bits 16-31 of handler address
} __attribute__((packed)) idt_entry_t;

// flags breakdown:
// Bit 7: Present (P) - must be 1 for valid entries
// Bit 6-5: Descriptor Privilege Level (DPL) - 0 for kernel, 3 for user
// Bit 4: Storage Segment (S) - must be 0 for interrupt gates
// Bit 3-0: Gate Type:
//   - 0x5 = Task Gate (32-bit)
//   - 0xE = Interrupt Gate (32-bit)
//   - 0xF = Trap Gate (32-bit)

typedef struct {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed)) idt_ptr_t;

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

idt_entry_t idt_entries[256];
idt_ptr_t idt_ptr;
bool vectors[IDT_MAX_DESCRIPTORS];
extern void *isr_stub_table[];

// populates an IDT entry with the address of an interrupt handler
void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags) {
  idt_entry_t *descriptor = &idt_entries[vector];

  descriptor->isr_low = (uint32_t)isr & 0xFFFF;
  descriptor->selector = 0x08; // this value can be whatever offset your kernel
                               // code selector is in your GDT
  descriptor->flags = flags;
  descriptor->isr_high = (uint32_t)isr >> 16;
  descriptor->reserved = 0;
}

static inline bool are_interrupts_enabled() {
  unsigned long flags;
  asm volatile("pushf\n\t"
               "pop %0"
               : "=g"(flags));
  return flags & (1 << 9);
}

extern void init_idt();
extern void exception_handler(interrupt_frame_t *frame);

void init_idt() {
  // Set up IDT register
  idt_ptr.base = (uintptr_t)&idt_entries[0];
  idt_ptr.limit = sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1;
  memset(&idt_entries, 0, sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS);

  // Remap PIC: exceptions use 0-31, so put hardware IRQs at 32-47
  pic_remap(32, 40); // 0x20, 0x28

  // Install exception handlers (0-31)
  for (uint8_t vector = 0; vector < 32; vector++) {
    idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
    vectors[vector] = true;
  }

  // Install IRQ handlers (32-47) - table indices 32-47
  for (uint8_t vector = 0; vector < 16; vector++) {
    idt_set_descriptor(32 + vector, isr_stub_table[32 + vector], 0x8E);
    vectors[32 + vector] = true; // Optional: Track these too
  }

  // Load new IDT
  __asm__ volatile("lidt %0" : : "m"(idt_ptr));

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

void test_software_interrupt(void) {
  __asm__ volatile("int $3"); // Triggers vector 3 in the IDT
}

void test_hardware_interrupt(void) {
  while (1) {
    __asm__ volatile("hlt"); // Low-power wait for interrupts
  }
}
