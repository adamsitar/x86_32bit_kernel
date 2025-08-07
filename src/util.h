#pragma once
#include <stdint.h>

void memset(void *destination, char value, uint32_t num_bytes) {
  char *ptr = (char *)destination;
  for (uint32_t i = 0; i < num_bytes; i++) {
    ptr[i] = value;
  }
}

void invalidate(uint32_t virtual_address) {
  asm volatile("invlpg %0" ::"m"(virtual_address));
}

#define CEIL_DIV(a, b) (((a + b) - 1) / b)
