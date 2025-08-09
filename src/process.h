#pragma once
#include <pfa.h>
#include <stdint.h>

typedef struct vma_block {
  uintptr_t start;
  size_t size; /* permissions, etc. */
  char name[25];
} vma_block_t;

vma_block_t user_memory_blocks[3];

void create_virtual_space() {
  uintptr_t new_pd_phys = pfa_alloc();
  temp_map(new_pd_phys);
  uint32_t *new_pd = (uint32_t *)TEMP_MAP_ADDR;
  memset(new_pd, 0, PAGE_SIZE);
  memcpy(&new_pd[768], &page_directory[768], (1024 - 768) * 4);
  uint32_t pde_index = (0x08000000 >> 22) & 0x3FF;
}
