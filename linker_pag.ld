/* The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */
ENTRY(_start)

/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS
{
	. = 0xC0100000; /* the code should be relocated to 3GB + 1MB */

	/* First put the multiboot header, as it is required to be put very early
	   in the image or the bootloader won't recognize the file format.
	   Next we'll put the .text section. */
	.text ALIGN(0x1000) : AT(ADDR(.text)-0xC0000000)
	{
		*(.multiboot)
		*(.text)
	}

	/* Read-only data. */
	.rodata ALIGN(0x1000) : AT(ADDR(.rodata) - 0xC0000000)
	{
		*(.rodata)
	}

	/* Read-write data (initialized) */
	.data ALIGN(0x1000) : AT(ADDR(.data) - 0xC0000000)
	{
		*(.data)
	}

	/* Read-write data (uninitialized) and stack */
	.bss  ALIGN(0x1000) : AT(ADDR(.bss) - 0xC0000000)
	{
		*(COMMON)
		*(.bss)
	}


  /* convenience setup so that within boot.asm we can load the inital paging setup more easily since it will already be in memory at a known address */
  /* page Directory: An array of 1024 x 32-bit entries (4KB total). Each entry points to a page table (for 4KB pages) or directly to a 4MB page */
  . = ALIGN(0x1000);  /* Ensure alignment */
  page_directory : AT(ADDR(page_directory) - 0xC0000000)
  {
      _page_directory = .;
      . += 0x1000;  /* 4KB for page directory */
  }

  /* Page Tables: Each is also 1024 x 32-bit entries (4KB). At least 2 are needed: */
  /* One for identity mapping (virtual 0x00000000–0x00400000 maps to physical 0x00000000–0x00400000). */
  . = ALIGN(0x1000);
  identity_page_table : AT(ADDR(identity_page_table) - 0xC0000000)
  {
      _identity_page_table = .;
      . += 0x1000;  /* 4KB for identity page table */
  }

  /* One for higher-half mapping (virtual 0xC0000000–0xC0400000 maps to physical 0x00000000–0x00400000, covering your kernel at physical 0x00100000+). */
  . = ALIGN(0x1000);
  higher_page_table : AT(ADDR(higher_page_table) - 0xC0000000)
  {
      _higher_page_table = .;
      . += 0x1000;  /* 4KB for higher-half page table */
  }

  /* all these are placed after the .bss at virtual addresses (e.g., 0xC01xxxxx), but the AT() ensures they're loaded at physical addresses (e.g., 0x00xxxxx). */
  /*  can reference symbols like _page_directory in assembly—but remember, before paging, you must use their physical addresses ( _page_directory - 0xC0000000 */
}
