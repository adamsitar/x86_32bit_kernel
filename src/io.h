#pragma once
#include <stdint.h>

// static inline void outb(uint16_t port, uint8_t val) {
//   __asm__ volatile("outb %b0, %1" : : "a"(val), "Nd"(port) : "memory");
// }
static inline void outb(uint16_t port, uint8_t val) {
  __asm__ volatile("outb %1, %0" : : "dN"(port), "a"(val));
}

static inline uint8_t inb(uint16_t port) {
  uint8_t value;
  __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port) : "memory");
  return value;
}

static inline void io_wait(void) { outb(0x80, 0); }

#define HIGH_ADDR 0xC0000000

// converts virtual to physical address
static inline uint32_t v_to_p(uint32_t addr) { return addr - HIGH_ADDR; }
static inline uint32_t p_to_v(uint32_t addr) { return addr + HIGH_ADDR; }
static inline uint32_t to_mb(uint32_t num) { return num >> 20; }
