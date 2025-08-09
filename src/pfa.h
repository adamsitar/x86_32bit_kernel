#pragma once
#include <bitmap.h>
#include <multiboot_gnu.h>
#include <pfa_helpers.h>
#include <printf.h>
#include <stdint.h>
#include <util.h>

#define PAGE_SIZE 4096
#define TEMP_MAP_ADDR                                                          \
  0xC03FF000 // As per book: Last entry in kernel's first PT (PDE 768, PTE 1023)
// #define TEMP_MAP_ADDR_BITSHIFT (768 << 22) | (1023 << 12) | 0x000
#define BITS_PER_BYTE 8

// Access boot.nasm structures
extern uint32_t page_directory[1024];
extern uint32_t
    page_table[1024]; // Kernel's initial PT (maps 0xC0000000 - 0xC0400000)

// Linker symbols for kernel physical range (add these to link.ld as extern)
extern char kernel_physical_start[];
extern char kernel_physical_end[];
extern uintptr_t physical_bitmap;

vm_bitmap_t vm_bitmap = {NULL, 0, 0, 0, 0};

// ============= PFA Initialization Steps =============

// Step 1: Initialize bitmap with all memory marked as used
static void pfa_init_bitmap(uint32_t num_frames) {
  vm_bitmap.bitmap_size = (num_frames + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
  vm_bitmap.bitmap = (uint8_t *)&physical_bitmap;

  // Initially mark everything as used (safer default)
  memset(vm_bitmap.bitmap, 0xFF, vm_bitmap.bitmap_size);

  printf("PFA: Initialized bitmap for %u frames (%u KB bitmap)\n", num_frames,
         vm_bitmap.bitmap_size / 1024);
}

// Step 2: Mark usable memory regions as free based on memory map
static void pfa_mark_usable_memory(multiboot_info_t *mbi) {
  multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)mbi->mmap_addr;
  uintptr_t mmap_end = mbi->mmap_addr + mbi->mmap_length;
  uint32_t usable_regions = 0;

  printf("PFA: Processing memory map...\n");

  while ((uintptr_t)mmap < mmap_end) {
    if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
      uint64_t base = mmap->addr;
      uint64_t length = mmap->len;

      // Only process memory below 4GB for 32-bit kernel
      if (base < 0x100000000ULL) {
        if (base + length > 0x100000000ULL) {
          length = 0x100000000ULL - base; // Truncate at 4GB
        }

        uint32_t start_frame = base / PAGE_SIZE;
        uint32_t num_frames = length / PAGE_SIZE;

        bitmap_mark_range_free(&vm_bitmap, start_frame, num_frames);
        usable_regions++;

        printf("  - Marked free: frames %u-%u (0x%x-0x%x)\n", start_frame,
               start_frame + num_frames - 1, (uint32_t)base,
               (uint32_t)(base + length - 1));
      }
    }

    mmap = (multiboot_memory_map_t *)((uintptr_t)mmap + mmap->size +
                                      sizeof(mmap->size));
  }

  printf("PFA: Marked %u usable regions as free\n", usable_regions);
}

// Step 3: Mark critical system areas as used
static void pfa_reserve_system_areas(void) {
  if (PRINT_MEMORY_MAP)
    printf("PFA: Reserving system areas...\n");

  // 1. Reserve NULL page (frame 0) - catch null pointer dereferences
  bitmap_set(vm_bitmap.bitmap, 0);
  if (PRINT_MEMORY_MAP)
    printf("  - Reserved: Frame 0 (NULL guard page)\n");

  // 2. Reserve BIOS data area (0x400-0x4FF)
  bitmap_set(vm_bitmap.bitmap, 0); // Frame 0 covers 0x0-0xFFF
  if (PRINT_MEMORY_MAP)
    printf("  - Reserved: BIOS IVT and data area\n");

  // 3. Reserve EBDA (typically at 0x9FC00 or 0x80000)
  // Usually already marked as reserved in memory map, but be explicit
  uint32_t ebda_frame = 0x9F000 / PAGE_SIZE;          // Typical EBDA location
  bitmap_mark_range_used(&vm_bitmap, ebda_frame, 16); // Reserve 64KB to be safe
  if (PRINT_MEMORY_MAP)
    printf("  - Reserved: Extended BIOS Data Area\n");

  // 4. Reserve VGA memory (0xA0000-0xBFFFF)
  uint32_t vga_start = 0xA0000 / PAGE_SIZE;
  uint32_t vga_frames = 0x20000 / PAGE_SIZE; // 128KB
  bitmap_mark_range_used(&vm_bitmap, vga_start, vga_frames);
  if (PRINT_MEMORY_MAP)
    printf("  - Reserved: VGA memory (0xA0000-0xBFFFF)\n");

  // 5. Reserve ROM area (0xC0000-0xFFFFF)
  uint32_t rom_start = 0xC0000 / PAGE_SIZE;
  uint32_t rom_frames = 0x40000 / PAGE_SIZE; // 256KB
  bitmap_mark_range_used(&vm_bitmap, rom_start, rom_frames);
  if (PRINT_MEMORY_MAP)
    printf("  - Reserved: BIOS ROM area (0xC0000-0xFFFFF)\n");
}

// Step 4: Mark kernel memory as used
static void pfa_reserve_kernel_memory(void) {
  // Calculate kernel's physical memory range
  uintptr_t kernel_start = (uintptr_t)&kernel_physical_start;
  uintptr_t kernel_end = (uintptr_t)&kernel_physical_end;

  // Also reserve some space after kernel for dynamic allocations
  uintptr_t reserved_end = kernel_end + (1024 * 1024); // +1MB after kernel

  uint32_t start_frame = kernel_start / PAGE_SIZE;
  uint32_t end_frame = (reserved_end + PAGE_SIZE - 1) / PAGE_SIZE;
  uint32_t num_frames = end_frame - start_frame;

  bitmap_mark_range_used(&vm_bitmap, start_frame, num_frames);

  printf("PFA: Reserved kernel memory:\n");
  printf("  - Physical: 0x%x-0x%x\n", kernel_start, reserved_end);
  printf("  - Frames: %u-%u (%u frames, %u KB)\n", start_frame, end_frame - 1,
         num_frames, (num_frames * 4));
}

// Step 5: Reserve multiboot structures
static void pfa_reserve_multiboot_structures(multiboot_info_t *mbi) {
  printf("PFA: Reserving multiboot structures...\n");

  // 1. Reserve the multiboot info structure itself
  uint32_t mbi_frame = ((uintptr_t)mbi) / PAGE_SIZE;
  bitmap_set(vm_bitmap.bitmap, mbi_frame);

  // 2. Reserve memory map area
  if (mbi->flags & MULTIBOOT_INFO_MEM_MAP) {
    uint32_t mmap_start_frame = mbi->mmap_addr / PAGE_SIZE;
    uint32_t mmap_pages = (mbi->mmap_length + PAGE_SIZE - 1) / PAGE_SIZE;
    bitmap_mark_range_used(&vm_bitmap, mmap_start_frame, mmap_pages);
  }

  // 3. Reserve modules if any
  if (mbi->flags & MULTIBOOT_INFO_MODS && mbi->mods_count > 0) {
    multiboot_module_t *mod = (multiboot_module_t *)mbi->mods_addr;
    for (uint32_t i = 0; i < mbi->mods_count; i++) {
      uint32_t mod_start_frame = mod[i].mod_start / PAGE_SIZE;
      uint32_t mod_end_frame = (mod[i].mod_end + PAGE_SIZE - 1) / PAGE_SIZE;
      bitmap_mark_range_used(&vm_bitmap, mod_start_frame,
                             mod_end_frame - mod_start_frame);
      printf("  - Reserved module %u: frames %u-%u\n", i, mod_start_frame,
             mod_end_frame - 1);
    }
  }

  // 4. Reserve command line if present
  if (mbi->flags & MULTIBOOT_INFO_CMDLINE) {
    uint32_t cmdline_frame = mbi->cmdline / PAGE_SIZE;
    bitmap_set(vm_bitmap.bitmap, cmdline_frame);
  }
}

// Step 6: Count available frames for statistics
static uint32_t pfa_count_free_frames(void) {
  uint32_t free_count = 0;

  for (uint32_t i = 0; i < vm_bitmap.total_frames; i++) {
    uint32_t byte_idx = i / 8;
    uint32_t bit_idx = i % 8;

    if ((vm_bitmap.bitmap[byte_idx] & (1 << bit_idx)) == 0) {
      free_count++;
    }
  }

  return free_count;
}

// ============= Main Initialization Function =============
void init_pfa(multiboot_info_t *mbi) {
  // printf("\n=== Initializing Page Frame Allocator ===\n");

  // Step 1: Parse memory map and determine total frames
  vm_bitmap.total_frames = get_max_usable_pages(mbi);
  if (vm_bitmap.total_frames == 0) {
    printf("PFA: No usable memory found!\n");
    return;
  }

  // Step 2: Initialize bitmap (all marked as used initially)
  pfa_init_bitmap(vm_bitmap.total_frames);

  // Step 3: Mark usable memory regions as free
  pfa_mark_usable_memory(mbi);

  // Step 4: Reserve critical system areas
  pfa_reserve_system_areas();

  // Step 5: Reserve kernel memory
  pfa_reserve_kernel_memory();

  // Step 6: Reserve multiboot structures
  pfa_reserve_multiboot_structures(mbi);

  // Step 7: Calculate and display statistics
  vm_bitmap.free_frames = pfa_count_free_frames();
  uint32_t used_frames = vm_bitmap.total_frames - vm_bitmap.free_frames;

  // printf("\n=== PFA Initialization Complete ===\n");
  // printf("Total frames: %u (%u MB)\n", vm_bitmap.vm_bitmap.total_frames,
  // (vm_bitmap.vm_bitmap.total_frames * 4) / 1024); printf("Free frames:  %u
  // (%u MB)\n", free_frames, (free_frames * 4) / 1024); printf("Used frames: %u
  // (%u MB)\n", used_frames, (used_frames * 4) / 1024); printf("Bitmap size: %u
  // bytes\n", vm_bitmap.bitmap_size);

  // Sanity check
  if (vm_bitmap.free_frames == 0) {
    printf("PFA: No free frames available after initialization!\n");
  }

  printf("PFA: Ready for allocations\n\n");
}

uintptr_t pfa_alloc() {
  for (uint32_t byte = 0; byte < vm_bitmap.bitmap_size; byte++) {
    if (vm_bitmap.bitmap[byte] !=
        0xFF) { // There is at least one free bit in this byte
      for (uint8_t bit = 0; bit < BITS_PER_BYTE; bit++) {
        uint8_t test_bit = 1 << bit;
        // Check if this bit is free (== 0)
        if ((vm_bitmap.bitmap[byte] & test_bit) == 0) {
          // Mark as used
          vm_bitmap.bitmap[byte] |= test_bit;
          uint32_t frame_num = (byte * BITS_PER_BYTE) + bit;
          uintptr_t phys_addr = frame_num * PAGE_SIZE;
          // Make sure we don't go past the max allowed physical address
          if (phys_addr >= vm_bitmap.max_phys_addr)
            return 0; // Out of physical memory

          // can't return frame 0, 0 is considered error code here
          if (phys_addr)
            return phys_addr;
        }
      }
    }
  }
  return 0; // Out of frames
}

void pfa_free(uintptr_t phys_addr) {
  if (phys_addr == 0 || phys_addr >= vm_bitmap.max_phys_addr)
    return;
  uint32_t frame_num = phys_addr / PAGE_SIZE;
  if (bitmap_test(vm_bitmap.bitmap, frame_num)) {
    bitmap_clear(vm_bitmap.bitmap, frame_num);
  }
}

static void temp_map(uintptr_t phys_addr) {
  page_table[1023] = (uint32_t)phys_addr | 3; // Present + writable
  asm volatile("invlpg %0"
               :
               : "m"(*(char *)TEMP_MAP_ADDR)); // Flush TLB for this addr
}

static void temp_unmap() {
  page_table[1023] = 0; // Unmap
  asm volatile("invlpg %0" : : "m"(*(char *)TEMP_MAP_ADDR));
}

// new page table for the virtual range starting at (pde_index * 4MB)
// function allocates a new page table (PT) in physical memory, initializes it,
// and links it into the current page directory (the global page_directory
// array) at a specific index
void alloc_new_pt(uint32_t *page_directory, uint32_t pde_index) {
  uintptr_t pt_phys = pfa_alloc();
  if (pt_phys == 0) {
    printf("OOM: Can't alloc new PT\n");
    return;
  }

  temp_map(pt_phys); // Map temporarily
  uint32_t *new_pt = (uint32_t *)TEMP_MAP_ADDR;
  // Zero the new PT, ensures all PTEs start invalid (not mapping anything)
  memset(new_pt, 0, PAGE_SIZE);

  // Writes the physical address of the new PT into the specified PDE index in
  // the current page directory
  page_directory[pde_index] = (uint32_t)pt_phys | 3;
  // sets the present (bit 0) and read/write (bit 1) flags
  // You might add User flag (bit 2) for user-mode access

  // full TLB flush, temporary due to inefficiency
  asm volatile("mov %%cr3, %%eax; mov %%eax, %%cr3" : : : "eax");

  temp_unmap(); // Clean up
  printf("New PT allocated at phys %p, mapped to PDE %u\n", pt_phys, pde_index);
}
