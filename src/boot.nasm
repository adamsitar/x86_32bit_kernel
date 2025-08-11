; Declare constants for the multiboot header.
MBALIGN  equ  1 << 0            ; align loaded modules on page boundaries
MEMINFO  equ  1 << 1            ; provide memory map
MBUSEGFX equ  0
MBFLAGS  equ  MBALIGN | MEMINFO | MBUSEGFX ; this is the Multiboot 'flag' field
MAGIC    equ  0x1BADB002        ; 'magic number' lets bootloader find the header
CHECKSUM equ -(MAGIC + MBFLAGS) ; checksum of above, to prove we are multiboot
                                ; CHECKSUM + MAGIC + MBFLAGS should be Zero (0)

section .multiboot
align 4
	dd MAGIC
	dd MBFLAGS
	dd CHECKSUM
  dd 0,0,0,0,0 ; read from the ELF so here is 0
  dd 0 ; near graphic mode
  dd 800 ; width
  dd 1000 ; height
  dd 32 ;depth

; Paging structures in low-linked BSS (accessible physically before paging)
section .bootstrap_bss
align 4096
global page_directory
page_directory:
    resb 4096  ; 1024 entries, 
    ;enough for 4gb of virtual space (1024 PTs * 1024 pages * 4kb = 4gb)
global page_table
page_table:
    resb 4096  ; one page table maps 4MB
    ; PFA must allocate frames for new PTs and link them to free PDFs (index 769+)

; Stack in higher-half BSS (set up after paging)
section .bss
align 16
global physical_bitmap
physical_bitmap:
    resb 131072  ; 128 KB = enough for 4GB at 4 KB/page
stack_bottom:
    resb 16384 * 8 
stack_top:


section .bootstrap
global _start
_start:
  cli ;  Disable interrupts (prevents early IRQs)

  ; Save magic (eax will be overwritten) to edx. ebx (info phys ptr) is untouched.
  mov edx, eax  ; edx = magic (preserved across jump)
 
  ; Step 1: Set up page table - map physical 0x0-0x003FFFFF (4mb)
  ; Either mapped as identity (0x0) or as 0xC0000000+, depending on how the PDE refers to them
  mov edi, page_table
  mov esi, 0  ; Start physical address 0x0
  mov ecx, 1024  ; we're mapping 1024 4kb pages (4MB)
  .loop_pt:
      mov eax, esi         ; EAX = current physical address
      or eax, 0x3          ; Flags: Present (1) + Writable (2)
      mov [edi], eax       ; Store in page table
      add esi, 0x1000      ; advance physical by 4kb
      add edi, 4           ; Next 4 byte page table entry
      loop .loop_pt        ; Repeat for all 1024 entries
  ; Step 2: Set up page directory
  mov eax, page_table ; PDE points to the page table
  or eax, 0x3  ; Present + writable
  mov [page_directory], eax  ; PDE 0 → virtual 0x00000000
  mov [page_directory + 768*4], eax  ; PDE 768 → virtual 0xC0000000 (768*4MB = 3GB)
  ; PDE[0] maps the first 4MB of virtual memory to the same physical address (identity mapping).
  ; PDE[768] maps 0xC0000000–0xC03FFFFF to those same physical addresses via the same PT
  ; Step 3: Load page directory into CR3
  mov eax, page_directory
  mov cr3, eax
  ; Step 4: Enable paging
  mov eax, cr0
  or eax, 0x80000000  ; Set paging bit
  mov cr0, eax
  ; Step 5: Jump to higher-half virtual address
  lea ecx, [higher_half]  ; Load virtual address
  jmp ecx
.end:

section .text
global higher_half
higher_half:
  ; edx == magic (preserved). ebx == info phys ptr (preserved).
  ; Convert info ptr to virtual (accessible via high mapping).
  add ebx, 0xC0000000

  ; Remove identity mapping (invalidate low virtual)
  ;lea ecx, [page_directory + 0xC0000000]  ; ecx = high virt addr of PDE 0
  ;mov dword [ecx], 0
  ;invlpg [0]
  
  ; Set up virtual stack
  mov esp, stack_top
  xor ebp, ebp 

  ; Push preserved values for kernel_main (matches Multiboot convention)
  push ebx  ; virtual info ptr
  push edx  ; magic

	extern kernel_main
	call kernel_main

	cli
.hang: hlt
	jmp .hang
