#pragma once
#include <stdint.h>

void memset(void *destination, char value, uint32_t num_bytes) {
  char *ptr = (char *)destination;
  for (uint32_t i = 0; i < num_bytes; i++) {
    ptr[i] = value;
  }
}

#define CEIL_DIV(a, b) (((a + b) - 1) / b)
