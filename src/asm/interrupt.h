#pragma once
#include <stdint.h>

typedef struct {
  uint16_t isr_low;   // The lower 16 bits of the ISR's address
  uint16_t kernel_cs; // The GDT segment selector that the CPU will load into CS
                      // before calling the ISR
  uint8_t reserved;   // Set to zero
  uint8_t attributes; // Type and attributes; see the IDT page
  uint16_t isr_high;  // The higher 16 bits of the ISR's address
} __attribute__((packed)) idt_entry_t;

// idt_entry_t idt[256] = {};

// to create an entry for a handler whose code starts at 0xDEADBEEF and that
// runs in privilege level 0 (therefore using the same code segment selector as
// the kernel)
// register the above example as an handler for interrupt 0 (divide-by-zero)
uint32_t idt[512] = {0xDEAD8E00, 0x0008BEEF};

// An entry in the IDT for an interrupt handler consists of 64 bits. The highest
// 32 bits:
// Bit:     | 31 16       | 15 | 14 13  | 12 | 11 | 10 9 8  | 7 6 5 | 4 3 2 1 0
// Content: | offset high | P | DPL     | 0 | D   | 1 1 0   | 0 0 0 | reserved

// The lowest 32 bits:
//  Bit:      | 31 16             | 15 0 |
//  Content:  | segment selector | offset low |

// offset high - The 16 highest bits of the 32 bit address in the segment.
// offset low - The 16 lowest bits of the 32 bits address in the segment.
// p - If the handler is present in memory or not (1 = present, 0 = not
// present).
// DPL Descriptor - Privilige Level, the privilege level the handler
// can be called from (0, 1, 2, 3).
// D - Size of gate, (1 = 32 bits, 0 = 16
// bits).
// segment selector - The offset in the GDT.
// r - Reserved.

struct cpu_state {
  unsigned int ebp;
  unsigned int edi;
  unsigned int esi;
  unsigned int edx;
  unsigned int ecx;
  unsigned int ebx;
  unsigned int eax;
  // No ESP field at all
} __attribute__((packed));

struct stack_state {
  unsigned int error_code;
  unsigned int eip;
  unsigned int cs;
  unsigned int eflags;
} __attribute__((packed));

void interrupt_handler(struct cpu_state cpu, struct stack_state stack,
                       unsigned int interrupt) {}
