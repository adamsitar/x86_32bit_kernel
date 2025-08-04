; Declare constants for the multiboot header.
MBALIGN  equ  1 << 0            ; align loaded modules on page boundaries
MEMINFO  equ  1 << 1            ; provide memory map
MBFLAGS  equ  MBALIGN | MEMINFO ; this is the Multiboot 'flag' field
MAGIC    equ  0x1BADB002        ; 'magic number' lets bootloader find the header
CHECKSUM equ -(MAGIC + MBFLAGS) ; checksum of above, to prove we are multiboot
                                ; CHECKSUM + MAGIC + MBFLAGS should be Zero (0)

section .multiboot
align 4
	dd MAGIC
	dd MBFLAGS
	dd CHECKSUM

; Paging structures in low-linked BSS (accessible physically before paging)
section .bootstrap_bss
align 4096
global page_directory
page_directory:
    resb 4096  ; Space for page directory (1024 entries)
global page_table
page_table:
    resb 4096  ; Space for one page table (maps 4MB)

; Stack in higher-half BSS (set up after paging)
section .bss
align 16
stack_bottom:
    resb 16384 ; 16 KiB stack
stack_top:

section .bootstrap
global _start
_start:
  cli ;  Disable interrupts NOW (prevents early IRQs)

	; MOD: Save multiboot magic (EAX) and info ptr (EBX) as we will overwrite registers.
	;push eax
	;push ebx
 
  ; Step 1: Set up page table (map physical 0x0-0x400000 to virtual 0x0 and 0xC0000000)
  mov edi, page_table
  mov esi, 0  ; Start at physical 0x0
  mov ecx, 1024  ; 1024 pages (4MB)
  .loop_pt:
      mov eax, esi         ; EAX = physical address
      or eax, 0x3          ; Flags: Present (1) + Writable (2)
      mov [edi], eax       ; Store in page table
      add esi, 0x1000      ; Next physical page
      add edi, 4           ; Next table entry
      loop .loop_pt        ; Repeat for all 1024 entries
  ; Step 2: Set up page directory
  mov eax, page_table
  or eax, 0x3  ; Present + writable
  mov [page_directory], eax  ; PDE 0: Map to virtual 0x0
  mov [page_directory + 768*4], eax  ; PDE 768: Map to virtual 0xC0000000 (768*4MB = 3GB)
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

; Main kernel entry (high-linked) after jump from bootstrap
section .text
global higher_half
higher_half:
  ; Optional: Remove identity mapping (invalidate low virtual)
  mov dword [page_directory], 0
  invlpg [0]
  ; Set up virtual stack
  mov esp, stack_top

  ; Adjust EBX (physical mbi pointer) to virtual (since low mem is mapped at 0xC0000000+)
  add ebx, 0xC0000000
  ; Push virtual mbi pointer as argument to kernel_main
  push ebx

	extern kernel_main
	call kernel_main

	cli
.hang: hlt
	jmp .hang
