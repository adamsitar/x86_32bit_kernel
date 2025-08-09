#pragma once
#include <printf.h>
#include <stdbool.h>
#include <stddef.h> // For size_t
#include <stdint.h> // For uint32_t, uintptr_t

#define PDE_COUNT 1024 // Fixed for 32-bit x86

extern uint32_t page_directory[1024];

typedef struct {
  uint32_t start;
  uint32_t end;
} pde_range_t;

bool is_pde_free(uint32_t pde) {
  return (pde & 1) == 0; // Bit 0 is the Present flag
}

size_t count_free_pdes(const uint32_t *pd) {
  if (pd == NULL) {
    printf("Error: NULL page directory\n");
    return 0;
  }

  size_t free_count = 0;
  for (size_t i = 0; i < PDE_COUNT; ++i) {
    if (is_pde_free(pd[i])) {
      ++free_count;
    }
  }

  return free_count;
}

int find_first_free_pde(const uint32_t *pd) {
  if (pd == NULL) {
    printf("Error: NULL page directory\n");
    return -1;
  }

  for (int i = 0; i < PDE_COUNT; ++i) {
    if (is_pde_free(pd[i])) {
      return i;
    }
  }

  return -1; // No free PDEs
}

size_t collect_free_pde_ranges(const uint32_t *pd, pde_range_t *ranges,
                               size_t max_ranges) {
  if (pd == NULL || ranges == NULL || max_ranges == 0) {
    printf("Error: Invalid arguments for collect_free_pde_ranges\n");
    return 0;
  }

  size_t range_count = 0;
  int start = -1;

  for (size_t i = 0; i < PDE_COUNT; ++i) {
    if (is_pde_free(pd[i])) {
      if (start == -1) {
        start = (int)i; // Start of a new range
      }

    } else {
      if (start != -1) {
        // End of a range
        if (range_count < max_ranges) {
          ranges[range_count].start = (uint32_t)start;
          ranges[range_count].end = (uint32_t)(i - 1);
          ++range_count;
        }

        start = -1;
      }
    }
  }

  // Handle trailing range if any
  if (start != -1 && range_count < max_ranges) {
    ranges[range_count].start = (uint32_t)start;
    ranges[range_count].end = PDE_COUNT - 1;
    ++range_count;
  }

  return range_count;
}

void print_pde_summary(const uint32_t *pd) {
  if (pd == NULL) {
    printf("Error: NULL page directory\n");
    return;
  }

  size_t free_count = count_free_pdes(pd);
  size_t used_count = PDE_COUNT - free_count;

  printf("PDE Summary:\n");
  printf("  Total PDEs: %u\n", PDE_COUNT);
  printf("  Used PDEs: %u\n", used_count);
  printf("  Free PDEs: %u\n", free_count);

  // Collect and print free ranges (allocate temp buffer; adjust size if needed)
  pde_range_t
      ranges[512]; // Enough for worst-case fragmentation (every other PDE free)
  size_t range_count = collect_free_pde_ranges(pd, ranges, 512);

  if (range_count == 0) {
    printf("  Free Ranges: None\n");
  } else {
    printf("  Free Ranges (%u total):\n", range_count);
    for (size_t i = 0; i < range_count; ++i) {
      if (ranges[i].start == ranges[i].end) {
        printf("    - Index %u\n", ranges[i].start);
      } else {
        printf("    - Indexes %u to %u (%u entries)\n", ranges[i].start,
               ranges[i].end, ranges[i].end - ranges[i].start + 1);
      }
    }
  }
}

int scan_pde_for_free(const uint32_t *pd, bool print_summary) {
  if (pd == NULL) {
    printf("Error: NULL page directory\n");
    return -1;
  }

  int first_free = find_first_free_pde(pd);

  if (print_summary) {
    print_pde_summary(pd);
  }

  if (first_free == -1) {
    printf("Warning: No free PDEs found\n");
  } else {
    printf("First free PDE index: %d\n", first_free);
  }

  return first_free;
}
