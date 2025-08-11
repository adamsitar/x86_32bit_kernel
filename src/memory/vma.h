#pragma once
#include <memory/pfa.h>
#include <stdint.h>
#include <util/bitmap.h>
#include <util/util.h>

#define PD_BASE_VADDR 0xFFC00000
#define GET_PT(pde_index) (PD_BASE_VADDR + (pde_index << 12))

vm_bitmap_t kernel_vm_bitmap;

uintptr_t vma_alloc(uint32_t *pd, size_t bytes, uintptr_t hint,
                    uint32_t flags) {
  if (bytes == 0)
    return 0;

  // Calculate required pages (ceiling)
  uint32_t num_pages = (bytes + PAGE_SIZE - 1) / PAGE_SIZE;

  // Find free virtual range (page index in bitmap)
  int32_t start_page_idx =
      bitmap_find_free_range(&kernel_vm_bitmap, num_pages, hint);
  if (start_page_idx == -1) {
    printf("VMM: No free virtual space for %u pages\n", num_pages);
    return 0;
  }

  // Calculate starting virtual address
  uintptr_t virt_start = (uintptr_t)start_page_idx * PAGE_SIZE;

  // Mark as used (before mapping, to reserve)
  bitmap_mark_range_used(&kernel_vm_bitmap, start_page_idx, num_pages);

  // For each page in the range: Ensure PDE/PT exists, alloc phys, set PTE
  for (uint32_t page = 0; page < num_pages; page++) {
    uintptr_t virt = virt_start + page * PAGE_SIZE;

    // Calculate PDE and PTE indexes
    uint32_t pde_index = virt >> 22;           // Top 10 bits
    uint32_t pte_index = (virt >> 12) & 0x3FF; // Next 10 bits

    // Ensure PT exists for this PDE
    if ((pd[pde_index] & 1) == 0) { // Check Present bit
      alloc_new_pt(pd, pde_index);
    }

    // Direct access to PT via recursive mapping
    uint32_t *pt = (uint32_t *)GET_PT(pde_index);

    // Alloc physical frame for this page
    uintptr_t page_phys = pfa_alloc();
    if (page_phys == 0) {
      // Rollback
      bitmap_mark_range_free(&kernel_vm_bitmap, start_page_idx, num_pages);
      return 0;
    }

    pt[pte_index] = (uint32_t)page_phys | 0b11; // Present (1) + R/W (2);

    invlpg(virt);
  }

  // TODO Store flags somewhere (e.g., in a separate VMA list for advanced
  // tracking)

  printf("VMA: Allocated %u pages at virt %p (hint %p, flags %x)\n", num_pages,
         virt_start, hint, flags);
  return virt_start;
}

void vma_free(uint32_t *pd, uintptr_t virt_start, size_t bytes) {
  if (bytes == 0 || virt_start == 0)
    return;

  uint32_t num_pages = (bytes + PAGE_SIZE - 1) / PAGE_SIZE;
  uint32_t start_page_idx = virt_start / PAGE_SIZE;

  // For each page, return phys to PFA
  for (uint32_t page = 0; page < num_pages; page++) {
    uintptr_t virt = virt_start + page * PAGE_SIZE;

    uint32_t pde_index = virt >> 22;
    uint32_t pte_index = (virt >> 12) & 0x3FF;

    if ((pd[pde_index] & 1) == 0)
      continue; // No PT - skip (shouldn't happen)

    uintptr_t pt_phys = pd[pde_index] & ~0xFFF;
    uint32_t *pt = (uint32_t *)GET_PT(pde_index);

    if ((pt[pte_index] & 1) == 0) { // Not present - skip
      continue;
    }

    // Get phys, clear PTE, return to PFA
    pt[pte_index] = 0; // Clear
    pfa_free(pt_phys); // Assume you have this to mark free in PFA bitmap

    invlpg(virt);
  }

  // Mark free in bitmap
  bitmap_mark_range_free(&kernel_vm_bitmap, start_page_idx, num_pages);

  // TODO If entire PT becomes empty, free it and clear PDE (optimize memory)

  printf("VMA: Freed %u pages at virt %p\n", num_pages, virt_start);
}
