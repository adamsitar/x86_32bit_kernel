; Declare constants for the multiboot header.
MBALIGN  equ  1 << 0            ; align loaded modules on page boundaries
MEMINFO  equ  1 << 1            ; provide memory map
MBFLAGS  equ  MBALIGN | MEMINFO ; this is the Multiboot 'flag' field
MAGIC    equ  0x1BADB002        ; 'magic number' lets bootloader find the header
CHECKSUM equ -(MAGIC + MBFLAGS) ; checksum of above, to prove we are multiboot
                                ; CHECKSUM + MAGIC + MBFLAGS should be Zero (0)

; Declare a multiboot header that marks the program as a kernel. These are magic
; values that are documented in the multiboot standard. The bootloader will
; search for this signature in the first 8 KiB of the kernel file, aligned at a
; 32-bit boundary. The signature is in its own section so the header can be
; forced to be within the first 8 KiB of the kernel file.
section .multiboot
align 4
	dd MAGIC
	dd MBFLAGS
	dd CHECKSUM

section .bootstrap
global _start
_start:
  cli ;  Disable interrupts NOW (prevents early IRQs)
	; The bootloader has loaded us into 32-bit protected mode on a x86
	; machine. Interrupts are disabled. Paging is disabled. The processor
	; state is as defined in the multiboot standard. The kernel has full
	; control of the CPU. The kernel can only make use of hardware features
	; and any code it provides as part of itself. There's no printf
	; function, unless the kernel provides its own <stdio.h> header and a
	; printf implementation. There are no security restrictions, no
	; safeguards, no debugging mechanisms, only what the kernel provides
	; itself. It has absolute and complete power over the
	; machine.

	; MOD: Save multiboot magic (EAX) and info ptr (EBX) as we will overwrite registers.
	push eax
	push ebx

	; MOD: Set up temporary low stack (before paging; cannot use high virtual stack_top yet).
	mov esp, 0x90000  ; Temporary stack in low memory (safe address below 1MB).

	; MOD: Set up paging (basic 4KB pages, map first 4MB identity and higher half).
	; Calculate physical addresses (virtual symbol - 0xC0000000).
	mov edi, page_table_kernel - 0xC0000000  ; Physical addr of page table.
	xor eax, eax                             ; Start physical page addr at 0.
	mov ecx, 1024                            ; 1024 entries in page table.
.fill_pt:
	mov [edi], eax
	or dword [edi], 3                        ; Flags: present + read/write.
	add eax, 4096                            ; Next 4KB page.
	add edi, 4                               ; Next entry.
	loop .fill_pt

	; MOD: Fill page directory (entry 0 for identity, entry 768 for 0xC0000000).
	mov eax, page_table_kernel - 0xC0000000  ; Physical PT addr + flags.
  or eax, 3
	mov edi, page_directory - 0xC0000000         ; Physical PD addr.
	mov [edi], eax                               ; PDE 0 (identity map).
	mov [edi + 768 * 4], eax                     ; PDE 768 (higher half map).

	; MOD: Load page directory into CR3 (using physical addr).
	mov eax, page_directory - 0xC0000000
	mov cr3, eax

	; MOD: Enable paging by setting bit 31 in CR0.
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax

	; MOD: Restore multiboot info temporarily to registers (EBX=ptr, EDX=magic).
	pop ebx  ; Multiboot info ptr (physical).
	pop edx  ; Multiboot magic.

	; MOD: Jump to higher half virtual address (now that paging is on).
	lea eax, [higher_half]  ; Load virtual addr of label.
	jmp eax                 ; Jump to it (switches execution to virtual high).
.end:

; The multiboot standard does not define the value of the stack pointer register
; (esp) and it is up to the kernel to provide a stack. This allocates room for a
; small stack by creating a symbol at the bottom of it, then allocating 16384
; bytes for it, and finally creating a symbol at the top. The stack grows
; downwards on x86. The stack is in its own section so it can be marked nobits,
; which means the kernel file is smaller because it does not contain an
; uninitialized stack. The stack on x86 must be 16-byte aligned according to the
; System V ABI standard and de-facto extensions. The compiler will assume the
; stack is properly aligned and failure to align the stack will result in
; undefined behavior.
section .bss
; MOD: Added paging structures (page directory and one table for 4MB mapping).
; These are aligned to 4096 (page size) and placed before the stack.
align 4096
global page_directory
page_directory:
resb 4096  ; Page directory (1024 entries * 4 bytes)

global page_table_kernel
page_table_kernel:
resb 4096  ; One page table (1024 entries * 4 bytes, maps 4MB)

; Original stack (unchanged, but now after paging structures).
align 16
stack_bottom:
resb 16384 ; 16 KiB is reserved for stack
stack_top:

; Main kernel entry (high-linked) after jump from bootstrap
section .text
global _start:function (_start.end - _start)
higher_half:
	; MOD: Now in higher half. Set real stack (virtual high address, now mapped).
	mov esp, stack_top

	; MOD: Restore multiboot params to expected registers for kernel_main (EAX=magic, EBX=ptr).
	mov eax, edx

	; This is a good place to initialize crucial processor state before the
	; high-level kernel is entered. It's best to minimize the early
	; environment where crucial features are offline. Note that the
	; processor is not fully initialized yet: Features such as floating
	; point instructions and instruction set extensions are not initialized
	; yet. The GDT should be loaded here. Paging should be enabled here.
	; C++ features such as global constructors and exceptions will require
	; runtime support to work as well.

	; Enter the high-level kernel. The ABI requires the stack is 16-byte
	; aligned at the time of the call instruction (which afterwards pushes
	; the return pointer of size 4 bytes). The stack was originally 16-byte
	; aligned above and we've since pushed a multiple of 16 bytes to the
	; stack since (pushed 0 bytes so far) and the alignment is thus
	; preserved and the call is well defined.
        ; note, that if you are building on Windows, C functions may have "_" prefix in assembly: _kernel_main
	extern kernel_main
	call kernel_main

	; If the system has nothing more to do, put the computer into an
	; infinite loop. To do that:
	; 1) Disable interrupts with cli (clear interrupt enable in eflags).
	;    They are already disabled by the bootloader, so this is not needed.
	;    Mind that you might later enable interrupts and return from
	;    kernel_main (which is sort of nonsensical to do).
	; 2) Wait for the next interrupt to arrive with hlt (halt instruction).
	;    Since they are disabled, this will lock up the computer.
	; 3) Jump to the hlt instruction if it ever wakes up due to a
	;    non-maskable interrupt occurring or due to system management mode.
	cli
.hang: hlt
	jmp .hang
