#pragma once
#include <stddef.h>
#include <stdint.h>

void memset(void *destination, char value, uint32_t num_bytes) {
  char *ptr = (char *)destination;
  for (uint32_t i = 0; i < num_bytes; i++) {
    ptr[i] = value;
  }
}

void *memcpy(void *destination, const void *source, uint32_t num_bytes) {
  char *dest_ptr = (char *)destination;
  const char *src_ptr = (const char *)source;
  for (uint32_t i = 0; i < num_bytes; i++) {
    dest_ptr[i] = src_ptr[i];
  }
  return destination;
}

void invlpg(uint32_t virtual_address) {
  asm volatile("invlpg %0" ::"m"(virtual_address));
}

#define CEIL_DIV(a, b) (((a + b) - 1) / b)
