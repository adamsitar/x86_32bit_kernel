#pragma once
#define BITS_PER_BYTE 8
#include <stddef.h>
#include <stdint.h>

typedef struct vm_bitmap {
  uint8_t *bitmap;      // Each bit represents one 4KB frame
  uint32_t bitmap_size; // In bytes
  uint32_t total_frames;
  uint32_t free_frames;
  uintptr_t max_phys_addr; // Highest usable physical addr
} vm_bitmap_t;

// Helper to set/clear/test bits
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
static void bitmap_mark_range_used(vm_bitmap_t vm_bitmap, uint32_t start_frame,
                                   uint32_t num_frames) {
  for (uint32_t i = 0;
       i < num_frames && (start_frame + i) < vm_bitmap.total_frames; i++) {
    bitmap_set(vm_bitmap.bitmap, start_frame + i);
  }
}

static void bitmap_mark_range_free(vm_bitmap_t vm_bitmap, uint32_t start_frame,
                                   uint32_t num_frames) {
  for (uint32_t i = 0;
       i < num_frames && (start_frame + i) < vm_bitmap.total_frames; i++) {
    bitmap_clear(vm_bitmap.bitmap, start_frame + i);
  }
}
