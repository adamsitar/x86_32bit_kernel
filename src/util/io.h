#pragma once
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) {
  __asm__ volatile("outb %1, %0" : : "dN"(port), "a"(val));
}

static inline uint8_t inb(uint16_t port) {
  uint8_t value;
  __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port) : "memory");
  return value;
}

static inline void io_wait(void) { outb(0x80, 0); }
