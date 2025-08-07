#pragma once
#include <stdint.h> // For uint32_t, uint16_t, etc.

// Sub-structure for a.out symbol table (if flags bit 4 is set)
typedef struct multiboot_aout_symbol_table {
  uint32_t tabsize;  // Size of the symbol table
  uint32_t strsize;  // Size of the string table
  uint32_t addr;     // Physical address of the symbol table
  uint32_t reserved; // Reserved (always 0)
} multiboot_aout_symbol_table_t;

// Sub-structure for ELF section header table (if flags bit 5 is set)
typedef struct multiboot_elf_section_header_table {
  uint32_t num;   // Number of section headers
  uint32_t size;  // Size of each section header entry
  uint32_t addr;  // Physical address of the section header table
  uint32_t shndx; // Index of the string table section
} multiboot_elf_section_header_table_t;

// Main Multiboot information structure.
// Check the 'flags' bitfield to see which parts are valid.
typedef struct multiboot_info {
  // Mandatory - always present
  uint32_t flags; // Bitfield indicating which of the following fields are
                  // valid. Bit 0: mem_* fields are valid Bit 1: boot_device is
                  // valid Bit 2: cmdline is valid Bit 3: mods_* are valid Bit
                  // 4: aout_sym is valid (a.out kernel symbols) Bit 5: elf_sec
                  // is valid (ELF kernel sections) Bit 6: mmap_* are valid
                  // (memory map) Bit 7: drives_* are valid Bit 8: config_table
                  // is valid Bit 9: boot_loader_name is valid Bit 10: apm_table
                  // is valid Bit 11: vbe_* are valid

  // Valid if flags bit 0 is set: Available memory from BIOS
  uint32_t mem_lower; // Amount of lower memory (below 1MB) in KB
  uint32_t mem_upper; // Amount of upper memory (above 1MB) in KB

  // Valid if flags bit 1 is set: BIOS boot device
  uint32_t boot_device; // BIOS disk device the kernel was loaded from
                        // Format: (bios_device, partition, sub_partition,
                        // sub_sub_partition) High byte: BIOS drive number
                        // (e.g., 0x80 for first HDD)

  // Valid if flags bit 2 is set: Kernel command line
  uint32_t cmdline; // Physical address of null-terminated command line string

  // Valid if flags bit 3 is set: Boot modules
  uint32_t mods_count; // Number of modules loaded
  uint32_t mods_addr;  // Physical address of the first module structure (array
                       // of multiboot_module_t)

  // Valid if flags bit 4 or 5 is set: Kernel symbol table info
  // This is a union: either a.out symbols (bit 4) or ELF sections (bit 5)
  union {
    multiboot_aout_symbol_table_t aout_sym;       // a.out format symbols
    multiboot_elf_section_header_table_t elf_sec; // ELF format sections
  } u; // Often named 'syms' in some docs

  // Valid if flags bit 6 is set: Memory map from BIOS
  uint32_t mmap_length; // Total size of the memory map buffer
  uint32_t mmap_addr;   // Physical address of the memory map buffer

  // Valid if flags bit 7 is set: BIOS drive info
  uint32_t drives_length; // Size of drives table
  uint32_t drives_addr;   // Physical address of first drive structure

  // Valid if flags bit 8 is set: ROM configuration table
  uint32_t config_table; // Physical address of ROM config table

  // Valid if flags bit 9 is set: Bootloader name
  uint32_t boot_loader_name; // Physical address of null-terminated bootloader
                             // name string

  // Valid if flags bit 10 is set: APM (Advanced Power Management) table
  uint32_t apm_table; // Physical address of APM table

  // Valid if flags bit 11 is set: VBE (VESA BIOS Extensions) graphics info
  uint32_t vbe_control_info;  // Physical address of VBE control info
  uint32_t vbe_mode_info;     // Physical address of VBE mode info
  uint16_t vbe_mode;          // Current VBE mode
  uint16_t vbe_interface_seg; // VBE interface segment
  uint16_t vbe_interface_off; // VBE interface offset
  uint16_t vbe_interface_len; // VBE interface length
} multiboot_info_t;

// For completeness: Structure of each module (pointed to by mods_addr)
// This is an array of these, with 'mods_count' entries.
typedef struct multiboot_module {
  uint32_t mod_start; // Physical start address of the module in memory
  uint32_t mod_end;   // Physical end address of the module
  uint32_t
      cmdline; // Physical address of null-terminated module command line string
  uint32_t reserved; // Padding/reserved (always 0)
} multiboot_module_t;

// Memory map entry structure (each entry in the buffer at mmap_addr)
typedef struct multiboot_mmap_entry {
  uint32_t size;        // Size of this structure (minus this field)
  uint32_t addr_low;    // Base address of memory region
  uint32_t addr_high;   // Base address of memory region
  uint32_t length_low;  // Length of memory region
  uint32_t length_high; // Length of memory region
#define MULTIBOOT_MEMORY_AVAILABLE 1
#define MULTIBOOT_MEMORY_RESERVED 2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE 3
#define MULTIBOOT_MEMORY_NVS 4
#define MULTIBOOT_MEMORY_BADRAM 5
  uint32_t type; // Type: 1=available RAM, 2=reserved, 3=ACPI reclaimable,
                 // 4=ACPI NVS, 5=bad
} __attribute__((
    packed)) multiboot_memory_map_t; // Packed to avoid alignment issues
