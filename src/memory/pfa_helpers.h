#pragma once
#include "multiboot_gnu.h"
#include "util/bitmap.h"
#include <stdint.h>
#include <util/printf.h>

#define PAGE_SIZE 4096

extern vm_bitmap_t vm_bitmap;

// Get human-readable memory type string
static const char *get_memory_type_string(uint32_t type) {
  switch (type) {
  case MULTIBOOT_MEMORY_AVAILABLE:
    return "Usable RAM";
  case MULTIBOOT_MEMORY_RESERVED:
    return "Reserved";
  case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
    return "ACPI Reclaimable";
  case MULTIBOOT_MEMORY_NVS:
    return "ACPI NVS";
  case MULTIBOOT_MEMORY_BADRAM:
    return "Bad RAM";
  default:
    return "Unknown";
  }
}

// Print a single memory region
static void print_memory_region(multiboot_memory_map_t *entry) {
  uint64_t base = entry->addr;
  uint64_t length = entry->len;
  uint64_t end = base + length;

  const char *type_str = get_memory_type_string(entry->type);

  // Handle display for 32-bit systems
  if (end > 0xFFFFFFFF) {
    // Region extends beyond 4GB
    printf("%s: 0x%x - 0x%x+ (extends beyond 4GB)\n", type_str, (uint32_t)base,
           0xFFFFFFFF);
  } else {
    // Normal region within 32-bit range
    printf("%s: 0x%x - 0x%x (%u KB)\n", type_str, (uint32_t)base,
           (uint32_t)(end - 1), // -1 to show last byte IN the region
           (uint32_t)(length / 1024));
  }
}

// Process memory map to find usable boundaries
static void find_usable_memory_bounds(multiboot_info_t *mbi,
                                      uint32_t *out_max_usable_addr,
                                      uint32_t *out_total_usable_kb) {
  uint32_t max_usable_addr = 0;
  uint32_t total_usable_kb = 0;

  multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)mbi->mmap_addr;
  uintptr_t mmap_end = mbi->mmap_addr + mbi->mmap_length;

  while ((uintptr_t)mmap < mmap_end) {
    if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
      uint64_t base = mmap->addr;
      uint64_t length = mmap->len;
      uint64_t end = base + length;

      // For 32-bit kernel, cap at 4GB
      if (end > 0xFFFFFFFF) {
        end = 0xFFFFFFFF;
        length = end - base;
      }

      // Track the highest usable address (within 32-bit range)
      if (end > max_usable_addr && base < 0xFFFFFFFF) {
        max_usable_addr = (uint32_t)end;
      }

      // Accumulate total usable memory
      total_usable_kb += (uint32_t)(length / 1024);
    }

    // Move to next entry
    mmap = (multiboot_memory_map_t *)((uintptr_t)mmap + mmap->size +
                                      sizeof(mmap->size));
  }

  *out_max_usable_addr = max_usable_addr;
  *out_total_usable_kb = total_usable_kb;
}

#define PRINT_MEMORY_MAP false
// Parse and display memory map
uint32_t get_max_usable_pages(multiboot_info_t *mbi) {
  if (!(mbi->flags & MULTIBOOT_INFO_MEM_MAP)) {
    printf("Error: No memory map provided by bootloader\n");
    return 0;
  }

  if (PRINT_MEMORY_MAP)
    printf("=== Memory Map ===\n");

  // First pass: Display all regions
  multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)mbi->mmap_addr;
  uintptr_t mmap_end = mbi->mmap_addr + mbi->mmap_length;

  while ((uintptr_t)mmap < mmap_end) {
    if (PRINT_MEMORY_MAP)
      print_memory_region(mmap);

    // Move to next entry
    mmap = (multiboot_memory_map_t *)((uintptr_t)mmap + mmap->size +
                                      sizeof(mmap->size));
  }

  // Second pass: Calculate usable memory statistics
  uint32_t max_usable_addr = 0;
  uint32_t total_usable_kb = 0;
  find_usable_memory_bounds(mbi, &max_usable_addr, &total_usable_kb);

  // Store global values
  vm_bitmap.max_phys_addr = max_usable_addr;
  vm_bitmap.total_frames = vm_bitmap.max_phys_addr / PAGE_SIZE;

  // Print summary
  if (PRINT_MEMORY_MAP) {
    printf("=== Memory Summary ===\n");
    printf("Highest usable address: 0x%x\n", vm_bitmap.max_phys_addr);
    printf("Total usable memory: %u KB (%u MB)\n", total_usable_kb,
           total_usable_kb / 1024);
    printf("Total page frames: %u\n", vm_bitmap.total_frames);
    printf("Bitmap size needed: %u bytes\n", (vm_bitmap.total_frames + 7) / 8);
  }

  return vm_bitmap.total_frames;
}
