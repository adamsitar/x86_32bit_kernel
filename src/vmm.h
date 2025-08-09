#pragma once
#include <bitmap.h>
#include <pfa.h>
#include <stdint.h>

vm_bitmap_t kernel_vm_bitmap; // Global for now; init at boot (e.g.,
                              // kernel_vm_bitmap.bitmap =
                              // early_alloc(BITMAP_BYTES); memset to 0)

// VMM Alloc: Returns starting virtual addr or NULL on failure
// Flags are stored but not used here (extend for policy, e.g., restrict based
// on flags)
uintptr_t vmm_alloc(uint32_t *pd, size_t bytes, uintptr_t hint,
                    uint32_t flags) {
  if (bytes == 0)
    return 0;

  // Calculate required pages (ceiling)
  uint32_t num_pages = (bytes + PAGE_SIZE - 1) / PAGE_SIZE;

  // Find free virtual range (page index in bitmap)
  int32_t start_page_idx = bitmap_find_free_range(
      &kernel_vm_bitmap, num_pages, hint); // Use global bitmap for now
  if (start_page_idx == -1) {
    // printf("VMM: No free virtual space for %u pages\n", num_pages);
    return 0; // OOM
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

    // Ensure PT exists for this PDE (alloc if not)
    if ((pd[pde_index] & 1) == 0) { // Check Present bit
      alloc_new_pt(pd,
                   pde_index); // This calls pfa_alloc internally for PT frame
    }

    // Get PT physical addr from PDE (mask off flags)
    uintptr_t pt_phys = pd[pde_index] & ~0xFFF;

    // Temp-map PT to access it
    temp_map(pt_phys);
    uint32_t *pt = (uint32_t *)TEMP_MAP_ADDR;

    // Alloc physical frame for this page
    uintptr_t page_phys = pfa_alloc();
    if (page_phys == 0) {
      // Rollback: Unmark and return failure
      bitmap_mark_range_free(&kernel_vm_bitmap, start_page_idx, num_pages);
      temp_unmap();
      return 0;
    }

    // Set PTE: phys addr | flags (e.g., Present + R/W; add User if flags
    // indicate)
    pt[pte_index] = (uint32_t)page_phys |
                    3; // 3 = Present (1) + R/W (2); customize based on flags

    // Flush TLB (simple full flush; optimize with invlpg later)
    asm volatile("mov %%cr3, %%eax; mov %%eax, %%cr3" : : : "eax");

    temp_unmap();
  }

  // TODO: Store flags somewhere (e.g., in a separate VMA list for advanced
  // tracking)

  // printf("VMM: Allocated %u pages at virt %p (hint %p, flags %x)\n",
  // num_pages, virt_start, hint, flags);
  return virt_start;
}

// VMM Free: Unmaps and frees the range; assumes exact match to a prior alloc
void vmm_free(uint32_t *pd, uintptr_t virt_start, size_t bytes) {
  if (bytes == 0 || virt_start == 0)
    return;

  // Calculate pages
  uint32_t num_pages = (bytes + PAGE_SIZE - 1) / PAGE_SIZE;
  uint32_t start_page_idx = virt_start / PAGE_SIZE;

  // For each page: Unmap PTE, return phys to PFA
  for (uint32_t page = 0; page < num_pages; page++) {
    uintptr_t virt = virt_start + page * PAGE_SIZE;

    uint32_t pde_index = virt >> 22;
    uint32_t pte_index = (virt >> 12) & 0x3FF;

    if ((pd[pde_index] & 1) == 0)
      continue; // No PT - skip (shouldn't happen)

    uintptr_t pt_phys = pd[pde_index] & ~0xFFF;
    temp_map(pt_phys);
    uint32_t *pt = (uint32_t *)TEMP_MAP_ADDR;

    if ((pt[pte_index] & 1) == 0) { // Not present - skip
      temp_unmap();
      continue;
    }

    // Get phys, clear PTE, return to PFA
    uintptr_t page_phys = pt[pte_index] & ~0xFFF;
    pt[pte_index] = 0; // Clear
    // pfa_free(page_phys);  // Assume you have this to mark free in PFA bitmap

    // Flush TLB
    asm volatile("mov %%cr3, %%eax; mov %%eax, %%cr3" : : : "eax");

    temp_unmap();
  }

  // Mark free in bitmap
  bitmap_mark_range_free(&kernel_vm_bitmap, start_page_idx, num_pages);

  // TODO: If entire PT becomes empty, free it and clear PDE (optimize memory)

  // printf("VMM: Freed %u pages at virt %p\n", num_pages, virt_start);
}
