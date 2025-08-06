#pragma once
#include <stdint.h>
#include <util.h>

// packed struct for a single GDT entry (8 bytes)
struct gdt_entry {
  uint16_t limit_low;  // Lower 16 bits of limit
  uint16_t base_low;   // Lower 16 bits of base
  uint8_t base_mid;    // Next 8 bits of base
  uint8_t access;      // Access flags (e.g., type, ring, present)
  uint8_t granularity; // Granularity, limit high bits, flags (e.g., 32-bit, 4KB
                       // pages)
  uint8_t base_high;   // Upper 8 bits of base
} __attribute__((packed));

// struct representing the GDT pointer (limit and base).
// must be packed to match the LGDT instruction's expectations:
// - 2 bytes for limit
// - 4 bytes for base address (32-bit)
struct gdt_ptr {
  uint16_t limit; // GDT limit (size - 1)
  uint32_t base;  // GDT base address
} __attribute__((packed));

// Global GDT array (minimal: null + code + data = 3 entries, 24 bytes)
static struct gdt_entry gdt[6];
struct gdt_ptr gp;
struct tss_entry tss_entry;

struct tss_entry {
  uint32_t prev_tss;
  uint32_t esp0; // stack pointer
  uint32_t ss0;  // kernel stack segment
  uint32_t esp1; // not used
  uint32_t ss1;
  uint32_t esp2;
  uint32_t ss2;
  uint32_t cr3;
  uint32_t eip;
  uint32_t eflags;
  uint32_t eax;
  uint32_t ecx;
  uint32_t edx;
  uint32_t ebx;
  uint32_t esp;
  uint32_t ebp;
  uint32_t esi;
  uint32_t edi;
  // segment registers
  uint32_t es;
  uint32_t cs;
  uint32_t ss;
  uint32_t ds;
  uint32_t fs;
  uint32_t gs;

  uint32_t ldt;
  uint32_t trap;
  uint32_t iomap_base;
} __attribute__((packed));

// set up a GDT entry
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

// void memset(void *dest, char val, uint32_t count);
void write_tss(uint32_t num, uint16_t ss0, uint32_t esp0) {
  uint32_t base = (uint32_t)&tss_entry;
  uint32_t limit = base + sizeof(tss_entry);

  gdt_set_entry(num, base, limit, 0xE9, 0x00);
  memset(&tss_entry, 0, sizeof(tss_entry));

  tss_entry.ss0 = esp0;
  tss_entry.ss0 = ss0;

  tss_entry.cs = 0x08 | 0x3; // 0x3 is the privilege level, to allow it to
                             // switch from user to kernel ring
  tss_entry.ss = tss_entry.ds = tss_entry.es = tss_entry.fs = tss_entry.gs =
      0x10 | 0x3;
};

extern void load_gdt(struct gdt_ptr *gdt);
extern void load_tss();
extern void flush_segments(uint16_t code_seg, uint16_t data_seg);

// initialize and load the minimal GDT
void init_gdt(void) {
  gp.limit = sizeof(gdt) - 1; // Limit is size of GDT minus 1
  gp.base = (uint32_t)&gdt;   // Base address of the GDT array

  // Null descriptor (index 0)
  gdt_set_entry(0, 0, 0, 0, 0);

  // Kernel code segment (index 1): base=0, limit=0xFFFFF (4GB), 32-bit, ring 0,
  // executable/readable
  // Access: 0x9A (present=1, ring=0, system=1, code=1, conforming=0,
  // executable=1, readable=1)
  // Granularity: 0xCF (gran=1 for 4KB pages, db=1 for 32-bit, limit_high=0xF)
  gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

  // Kernel data segment (index 2): base=0, limit=0xFFFFF (4GB), 32-bit, ring 0,
  // writable
  // Access: 0x92 (present=1, ring=0, system=1, code=0, direction=0,
  // executable=0, writable=1)
  // Granularity: 0xCF (same as above)
  gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

  // User code segment
  gdt_set_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
  // User data segment
  gdt_set_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

  // 0x10 offset of gdt table
  write_tss(5, 0x10, 0x0);

  // Load the GDT
  load_gdt(&gp);
  load_tss();

  // Flush segment registers (code selector 0x08, data selector 0x10)
  flush_segments(0x08, 0x10);
}
