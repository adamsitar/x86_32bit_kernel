#pragma once

/** outb:
 * Sends the given data to the given I/O port. Defined in io.s
 *
 * @param port The I/O port to send the data to
 * @param data The data to send to the I/O port
 */
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) {
  __asm__ volatile("outb %b0, %1" : : "a"(val), "Nd"(port) : "memory");
  /* There's an outb %al, $imm8 encoding, for compile-time constant port numbers
   * that fit in 8b. (N constraint). Wider immediate constants would be
   * truncated at assemble-time (e.g. "i" constraint). The  outb  %al, %dx
   * encoding is the only option for all other cases. %1 expands to %dx because
   * port  is a uint16_t.  %w1 could be used if we had the port number a wider C
   * type */
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
