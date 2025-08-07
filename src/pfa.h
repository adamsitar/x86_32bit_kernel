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
extern uintptr_t physical_bitmap;

static uint8_t *pfa_bitmap = NULL; // Each bit represents one 4KB frame
static uint32_t bitmap_size = 0;   // In bytes
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

uint32_t get_max_usable_pages(multiboot_info_t *mbi) {
  multiboot_mmap_entry_t *mmap = (multiboot_mmap_entry_t *)mbi->mmap_addr;
  uintptr_t usable_start = 0xFFFFFFFF;
  uintptr_t usable_end = 0;

  printf("Parsing Multiboot memory map...\n");
  const uintptr_t start_address = mbi->mmap_addr;
  const uintptr_t end_address = mbi->mmap_addr + mbi->mmap_length;
  const size_t memory_block_size = mmap->size + sizeof(mmap->size);
  for (uintptr_t current_address = start_address; current_address < end_address;
       current_address += memory_block_size) {
    mmap = (multiboot_mmap_entry_t *)current_address;
    // create addresses from two 16 bit chunks
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
  return total_frames;
}

void init_pfa(multiboot_info_t *mbi) {
  // Step 1: Parse Multiboot memory map to find usable RAM and max addr
  multiboot_mmap_entry_t *mmap = (multiboot_mmap_entry_t *)mbi->mmap_addr;
  uint32_t total_frames = get_max_usable_pages(mbi);

  // Step 2: Allocate bitmap (one bit per frame) from initial kernel space
  bitmap_size = (total_frames + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
  pfa_bitmap =
      (uint8_t
           *)&physical_bitmap; // Example: Alloc from end of initial 4MB mapping
                               // (adjust to free space in your kernel)
  memset(pfa_bitmap, 0xFF, bitmap_size); // Mark all as allocated initially

  // Step 3: Mark usable regions as free in bitmap
  mmap = (multiboot_mmap_entry_t *)mbi->mmap_addr;
  const uintptr_t start_address = mbi->mmap_addr;
  const uintptr_t end_address = mbi->mmap_addr + mbi->mmap_length;
  const size_t memory_block_size = mmap->size + sizeof(mmap->size);
  for (uintptr_t current_address = start_address; current_address < end_address;
       current_address += memory_block_size) {
    mmap = (multiboot_mmap_entry_t *)current_address;
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

uintptr_t pfa_alloc() {
  for (uint32_t byte = 0; byte < bitmap_size; byte++) {
    if (pfa_bitmap[byte] !=
        0xFF) { // There is at least one free bit in this byte
      for (uint8_t bit = 0; bit < BITS_PER_BYTE; bit++) {
        uint8_t test_bit = 1 << bit;
        // Check if this bit is free (== 0)
        if ((pfa_bitmap[byte] & test_bit) == 0) {
          // Mark as used
          pfa_bitmap[byte] |= test_bit;
          uint32_t frame_num = (byte * BITS_PER_BYTE) + bit;
          uintptr_t phys_addr = frame_num * PAGE_SIZE;
          // Make sure we don't go past the max allowed physical address
          if (phys_addr >= max_phys_addr)
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
  if (phys_addr == 0 || phys_addr >= max_phys_addr)
    return;
  uint32_t frame_num = phys_addr / PAGE_SIZE;
  if (bitmap_test(frame_num)) {
    bitmap_clear(frame_num);
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

// Example: Alloc and add a new PT to PDE 'index' (e.g., 769 for next 4MB)
void alloc_new_pt(uint32_t pde_index) {
  uintptr_t pt_phys = pfa_alloc();
  if (pt_phys == 0) {
    printf("OOM: Can't alloc new PT\n");
    return;
  }

  temp_map(pt_phys); // Map temporarily
  uint32_t *new_pt = (uint32_t *)TEMP_MAP_ADDR;
  memset(new_pt, 0, PAGE_SIZE); // Zero the new PT

  // Optional: Fill some entries (e.g., identity map or kernel mappings)
  // for (int i = 0; i < 1024; i++) new_pt[i] = (some_phys + i*PAGE_SIZE) | 3;

  page_directory[pde_index] = (uint32_t)pt_phys | 3; // Link to PDT
  asm volatile("mov %%cr3, %%eax; mov %%eax, %%cr3"
               :
               :
               : "eax"); // Full TLB flush

  temp_unmap(); // Clean up
  printf("New PT allocated at phys %p, mapped to PDE %u\n", pt_phys, pde_index);
}
