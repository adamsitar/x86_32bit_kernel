#include "multiboot.h"
#include "terminal.h"
#include "util.h"

// Convert physical address to virtual (add 0xC0000000)
#define PHYS_TO_VIRT(addr) ((uintptr_t)(addr) + 0xC0000000)

#define PAGE_SIZE 4096
#define TEMP_MAP_ADDR                                                          \
  0xC03FF000 // As per book: Last entry in kernel's first PT (PDE 768, PTE 1023)
#define BITS_PER_BYTE 8

// Access boot.nasm structures
extern uint32_t page_directory[1024];
extern uint32_t
    page_table[1024]; // Kernel's initial PT (maps 0xC0000000 - 0xC0400000)

// Linker symbols for kernel physical range (add these to link.ld as extern)
extern uintptr_t kernel_physical_start;
extern uintptr_t kernel_physical_end;

static uint8_t *pfa_bitmap = NULL;
static uint32_t bitmap_size = 0; // In bytes
static uint32_t total_frames = 0;
static uintptr_t max_phys_addr = 0; // Highest usable physical addr

// Helper to set/clear/test bits
static void bitmap_set(uint32_t bit) {
  pfa_bitmap[bit / BITS_PER_BYTE] |= (1 << (bit % BITS_PER_BYTE));
}

static void bitmap_clear(uint32_t bit) {
  pfa_bitmap[bit / BITS_PER_BYTE] &= ~(1 << (bit % BITS_PER_BYTE));
}

static int bitmap_test(uint32_t bit) {
  return pfa_bitmap[bit / BITS_PER_BYTE] & (1 << (bit % BITS_PER_BYTE));
}

extern uintptr_t physical_bitmap;

void init_pfa(multiboot_info_t *mbi) {
  // Step 1: Parse Multiboot memory map to find usable RAM and max addr
  multiboot_mmap_entry_t *mmap = (multiboot_mmap_entry_t *)mbi->mmap_addr;
  uintptr_t usable_start = 0xFFFFFFFF;
  uintptr_t usable_end = 0;

  printf("Parsing Multiboot memory map...\n");
  for (uintptr_t addr = mbi->mmap_addr;
       addr < mbi->mmap_addr + mbi->mmap_length;
       addr += mmap->size + sizeof(mmap->size)) {
    mmap = (multiboot_mmap_entry_t *)addr;
    uintptr_t region_start =
        mmap->addr_low | ((uintptr_t)mmap->addr_high << 32);
    uintptr_t region_end =
        region_start +
        (mmap->length_low | ((uintptr_t)mmap->length_high << 32));

    if (mmap->type == 1) { // Usable RAM
      if (region_start < usable_start)
        usable_start = region_start;
      if (region_end > usable_end)
        usable_end = region_end;
      printf("Usable RAM: %p - %p\n", region_start, region_end);
    } else {
      printf("Reserved: %p - %p\n", region_start, region_end);
    }
  }

  max_phys_addr = usable_end;
  total_frames = max_phys_addr /
                 PAGE_SIZE; // Total possible frames (may overcount non-usable)
  printf("Total frames: %u\n", total_frames);

  // Step 2: Allocate bitmap (one bit per frame) from initial kernel space
  bitmap_size = (total_frames + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
  pfa_bitmap =
      (uint8_t
           *)&physical_bitmap; // Example: Alloc from end of initial 4MB mapping
                               // (adjust to free space in your kernel)
  memset(pfa_bitmap, 0xFF, bitmap_size); // Mark all as allocated initially

  // Step 3: Mark usable regions as free in bitmap
  mmap = (multiboot_mmap_entry_t *)PHYS_TO_VIRT(mbi->mmap_addr);
  for (uintptr_t addr = PHYS_TO_VIRT(mbi->mmap_addr);
       addr < PHYS_TO_VIRT(mbi->mmap_addr + mbi->mmap_length);
       addr += mmap->size + sizeof(mmap->size)) {
    mmap = (multiboot_mmap_entry_t *)addr;
    if (mmap->type == 1) {
      uintptr_t start_frame = mmap->addr_low / PAGE_SIZE;
      uintptr_t num_frames = mmap->length_low / PAGE_SIZE;
      for (uint32_t i = 0; i < num_frames; i++) {
        bitmap_clear(start_frame + i);
      }
    }
  }

  // Step 4: Mark kernel's physical range as used (from linker symbols)
  uintptr_t kernel_start_frame = kernel_physical_start / PAGE_SIZE;
  uintptr_t kernel_num_frames =
      (kernel_physical_end - kernel_physical_start + PAGE_SIZE - 1) / PAGE_SIZE;
  for (uint32_t i = 0; i < kernel_num_frames; i++) {
    bitmap_set(kernel_start_frame + i);
  }

  // Optional: Mark other reserved areas (e.g., Multiboot structures, modules)
  printf("PFA initialized: %u frames, bitmap size %u bytes\n", total_frames,
         bitmap_size);
}

void invalidate(uint32_t virtual_address) {
  asm volatile("invlpg %0" ::"m"(virtual_address));
}
