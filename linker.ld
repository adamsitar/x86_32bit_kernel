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

  . = 0x00100000;
  kernel_physical_start = .;
  /* bootstrap section for low-linked boot code (VMA = LMA = 0x00100000). */
	.bootstrap ALIGN(4K) : AT(ADDR(.bootstrap))
	{
		*(.multiboot)
		*(.bootstrap)
	}


	/* MOD: Compute size and physical end of bootstrap to avoid LMA overlap with later sections. */
	bootstrap_size = SIZEOF(.bootstrap);
	bootstrap_physical_end = kernel_physical_start + bootstrap_size;

	/* MOD: Set kernel VMA to 0xC0000000 + bootstrap_physical_end (e.g., 0xC0100100 if bootstrap=0x100 bytes).
	   This ensures paging maps virtual addresses correctly to the shifted physical addresses. */
	. = 0xC0000000 + bootstrap_physical_end;
	kernel_virtual_start = .;

	/* MOD: Added AT() to specify physical load address (LMA) at 0x00100000. */
  /* .text now after bootstrap, with low LMA via AT(). */
	.text ALIGN(4K) : AT(ADDR(.text) - 0xC0000000)
	{
		*(.text)
	}

	/* MOD: Added AT(ADDR(...) - 0xC0000000) to compute physical LMA based on virtual VMA. */
	.rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xC0000000)
	{
		*(.rodata)
	}

	/* MOD: Added AT(ADDR(...) - 0xC0000000) to compute physical LMA based on virtual VMA. */
	.data ALIGN(4K) : AT(ADDR(.data) - 0xC0000000)
	{
		*(.data)
	}

	/* MOD: Added AT(ADDR(...) - 0xC0000000) to compute physical LMA based on virtual VMA. */
	.bss ALIGN(4K) : AT(ADDR(.bss) - 0xC0000000)
	{
		*(COMMON)
		*(.bss)
	}

	/* The compiler may produce other sections, by default it will put them in
	   a segment with the same name. Simply add stuff here as needed. */

	/* MOD: Added symbols for kernel end (useful for later memory management). */
	kernel_virtual_end = .;
	kernel_physical_end = . - 0xC0000000;
}
