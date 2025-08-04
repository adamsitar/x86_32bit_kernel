/* The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */
ENTRY(_start)

/* readelf -h bin/os.bin | grep Entry  = this shows the entry point address */
/* entrypoint that was just set with the ENTRY() */
/* which is the VMA, not the LMA */


/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS
{
  /* linkers location counter sets the VMA - this is what symbols resolve to*/
  . = 0x00100000;
  kernel_physical_start = .;

  /* If you omit AT(), the LMA defaults to the current .  */
  /* AT expression explicitly sets the LMA, it does not affect the VMA */
  /* the VMA is controlled separately by the . location counter */
  /* Low-linked bootstrap section (VMA = LMA = 0x00100000). */
	.bootstrap ALIGN(4K) : AT(ADDR(.bootstrap))
	{
		*(.multiboot)
		*(.bootstrap)
	}

  /* Low-linked BSS for paging structures (accessible physically). */
  .bootstrap_bss ALIGN(4K) : AT(ADDR(.bootstrap_bss))
  {
    *(.bootstrap_bss)
  }

  /* Compute physical end after bootstrap + its BSS. */
  kernel_physical_end = .;
  /* Set kernel virtual start to 0xC0100000 (standard; maps to phys 0x100000 via page table). */
  . = 0xC0000000 + kernel_physical_end;
  kernel_virtual_start = .;

  /* Higher-half sections with physical LMAs computed from VMAs. */
	.text ALIGN(4K) : AT(kernel_physical_end + (ADDR(.text) - kernel_virtual_start))
	{
		*(.text)
	}

	/* MOD: Added AT(ADDR(...) - 0xC0000000) to compute physical LMA based on virtual VMA. */
	.rodata ALIGN(4K) : AT(kernel_physical_end + (ADDR(.rodata) - kernel_virtual_start))
	{
		*(.rodata)
	}

	/* MOD: Added AT(ADDR(...) - 0xC0000000) to compute physical LMA based on virtual VMA. */
	.data ALIGN(4K) : AT(kernel_physical_end + (ADDR(.data) - kernel_virtual_start))
	{
		*(.data)
	}

	/* MOD: Added AT(ADDR(...) - 0xC0000000) to compute physical LMA based on virtual VMA. */
	.bss ALIGN(4K) : AT(kernel_physical_end + (ADDR(.bss) - kernel_virtual_start))
	{
		*(COMMON)
		*(.bss)
	}

	/* MOD: Added symbols for kernel end (useful for later memory management). */
	kernel_virtual_end = .;
}
