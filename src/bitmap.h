#pragma once
#include <stddef.h>
#include <stdint.h>

#define BITS_PER_BYTE 8

// Constants
#define PAGE_SIZE 4096
#define PAGES_PER_PT 1024              // 1024 PTEs per PT (4MB)
#define NUM_PDES 1024                  // 1024 PDEs in PD (4GB)
#define BITMAP_SIZE (1 << 20)          // 1M pages in 4GB space (2^20 bits)
#define BITMAP_WORDS (BITMAP_SIZE / 8) // Bytes needed for bitmap (128KB)

typedef struct vm_bitmap {
  uint8_t *bitmap;      // Each bit represents one 4KB frame
  uint32_t bitmap_size; // In bytes
  uint32_t total_frames;
  uint32_t free_frames;
  uintptr_t max_phys_addr;
} vm_bitmap_t;

static void bitmap_set(uint8_t *bitmap, uint32_t bit) {
  bitmap[bit / BITS_PER_BYTE] |= (1 << (bit % BITS_PER_BYTE));
}

static void bitmap_clear(uint8_t *bitmap, uint32_t bit) {
  bitmap[bit / BITS_PER_BYTE] &= ~(1 << (bit % BITS_PER_BYTE));
}

static int bitmap_test(uint8_t *bitmap, uint32_t bit) {
  return bitmap[bit / BITS_PER_BYTE] & (1 << (bit % BITS_PER_BYTE));
}

// ============= Bitmap Helper Functions =============
static void bitmap_mark_range_used_old(vm_bitmap_t vm_bitmap,
                                       uint32_t start_frame,
                                       uint32_t num_frames) {
  for (uint32_t i = 0;
       i < num_frames && (start_frame + i) < vm_bitmap.total_frames; i++) {
    bitmap_set(vm_bitmap.bitmap, start_frame + i);
  }
}

void bitmap_mark_range_used(vm_bitmap_t *bitmap, uint32_t start_page,
                            uint32_t num_pages) {
  for (uint32_t i = start_page; i < start_page + num_pages; i++) {
    uint32_t byte = i / 8;
    uint32_t bit = i % 8;
    bitmap->bitmap[byte] |= (1 << bit);
  }
}

static void bitmap_mark_range_free_old(vm_bitmap_t vm_bitmap,
                                       uint32_t start_frame,
                                       uint32_t num_frames) {
  for (uint32_t i = 0;
       i < num_frames && (start_frame + i) < vm_bitmap.total_frames; i++) {
    bitmap_clear(vm_bitmap.bitmap, start_frame + i);
  }
}

void bitmap_mark_range_free(vm_bitmap_t *bitmap, uint32_t start_page,
                            uint32_t num_pages) {
  for (uint32_t i = start_page; i < start_page + num_pages; i++) {
    uint32_t byte = i / 8;
    uint32_t bit = i % 8;
    bitmap->bitmap[byte] &= ~(1 << bit);
  }
}

static int32_t bitmap_find_free_range(vm_bitmap_t *bitmap, uint32_t num_pages,
                                      uintptr_t hint) {
  uint32_t start_page =
      (hint == 0) ? (0xC0000000 / PAGE_SIZE)
                  : (hint / PAGE_SIZE); // Default to high kernel space
  uint32_t total_pages = BITMAP_SIZE;

  for (uint32_t i = start_page; i < total_pages; i++) {
    uint32_t free_count = 0;
    for (uint32_t j = i; j < i + num_pages && j < total_pages; j++) {
      uint32_t byte = j / 8;
      uint32_t bit = j % 8;
      if (bitmap->bitmap[byte] & (1 << bit)) {
        break; // Not free
      }
      free_count++;
    }
    if (free_count == num_pages) {
      return i; // Found starting page index
    }
    i += free_count; // Skip to next potential start (optimize scan)
  }
  // Wrap around if hint was high (optional; for simplicity, no wrap here)
  return -1; // No space found
}
