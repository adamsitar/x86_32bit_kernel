#pragma once

/** outb:
 * Sends the given data to the given I/O port. Defined in io.s
 *
 * @param port The I/O port to send the data to
 * @param data The data to send to the I/O port
 */
#include <stdint.h>
void outb(unsigned short port, unsigned char data);

/**
 * Struct representing the GDT pointer (limit and base).
 * This must be packed to match the LGDT instruction's expectations:
 * - 2 bytes for limit
 * - 4 bytes for base address (32-bit)
 */
struct gdt_ptr {
  uint16_t limit; // GDT limit (size - 1)
  uint32_t base;  // GDT base address
} __attribute__((packed));

/**
 * Loads the Global Descriptor Table (GDT) using the provided pointer.
 *
 * @param gdt Pointer to the GDT struct (containing limit and base).
 */
void load_gdt(struct gdt_ptr *gdt);

/**
 * Flushes the segment registers after loading the GDT.
 * This sets DS, ES, FS, GS, SS to the data segment and performs a far jump to
 * set CS.
 *
 * Call this immediately after load_gdt() to apply the changes.
 *
 * @param code_seg The code segment selector (e.g., 0x08 for a typical kernel
 * code segment).
 * @param data_seg The data segment selector (e.g., 0x10 for a typical kernel
 * data segment).
 */
extern void flush_segments(uint16_t code_seg, uint16_t data_seg);

// Define a packed struct for a single GDT entry (8 bytes)
struct gdt_entry {
  uint16_t limit_low;  // Lower 16 bits of limit
  uint16_t base_low;   // Lower 16 bits of base
  uint8_t base_mid;    // Next 8 bits of base
  uint8_t access;      // Access flags (e.g., type, ring, present)
  uint8_t granularity; // Granularity, limit high bits, flags (e.g., 32-bit, 4KB
                       // pages)
  uint8_t base_high;   // Upper 8 bits of base
} __attribute__((packed));

// Global GDT array (minimal: null + code + data = 3 entries, 24 bytes)
static struct gdt_entry gdt[3];

// Function to set up a GDT entry
static void gdt_set_entry(int index, uint32_t base, uint32_t limit,
                          uint8_t access, uint8_t gran) {
  struct gdt_entry *entry = &gdt[index];
  entry->base_low = (base & 0xFFFF);
  entry->base_mid = (base >> 16) & 0xFF;
  entry->base_high = (base >> 24) & 0xFF;
  entry->limit_low = (limit & 0xFFFF);
  entry->granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
  entry->access = access;
}

// Function to initialize and load the minimal GDT
void init_gdt(void) {
  // Null descriptor (index 0)
  gdt_set_entry(0, 0, 0, 0, 0);

  // Code segment (index 1): base=0, limit=0xFFFFF (4GB), 32-bit, ring 0,
  // executable/readable Access: 0x9A (present=1, ring=0, system=1, code=1,
  // conforming=0, executable=1, readable=1) Granularity: 0xCF (gran=1 for 4KB
  // pages, db=1 for 32-bit, limit_high=0xF)
  gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

  // Data segment (index 2): base=0, limit=0xFFFFF (4GB), 32-bit, ring 0,
  // writable Access: 0x92 (present=1, ring=0, system=1, code=0, direction=0,
  // executable=0, writable=1) Granularity: 0xCF (same as above)
  gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

  // Set up the GDT pointer
  struct gdt_ptr gp;
  gp.limit = sizeof(gdt) - 1; // Limit is size of GDT minus 1
  gp.base = (uint64_t)&gdt;   // Base address of the GDT array

  // Load the GDT
  load_gdt(&gp);

  // Flush segment registers (code selector 0x08, data selector 0x10)
  flush_segments(0x08, 0x10);
}
