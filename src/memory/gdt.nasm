global load_gdt
; stack: [esp + 4] the address of the GDT struct (limit:2 bytes, base:4 bytes)
; [esp] return address
load_gdt:
  mov eax, [esp + 4] ; move address of GDT struct (which was passed into argument) into eax
  lgdt [eax]         ; load the GDT
  ret                ; return to the calling function

global load_tss
load_tss:
  mov ax, 0x2B ; the offset associated with the task segment
  ; the index of the structure is at 0x28 since it's the 5th thing in the GDT
  ; we actually set it to the bottom 2 bits of that entry, so the bottom 2 bits are at 0x2B
  ltr ax
  ret

global flush_segments
; flush_segments - Load segment registers after GDT is set up
; updates DS, ES, FS, GS, SS to the data segment, and does a far jump to update CS.
; stack: [esp + 8] data_seg (uint16_t, e.g., 0x10)
;        [esp + 4] code_seg (uint16_t, e.g., 0x08)
;        [esp] return address
flush_segments:
    mov ax, [esp + 8]    ; Load data_seg into AX (selectors are 16-bit)
    ; Load data segment registers
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ; Perform far jump to update CS (push code_seg and target address, then retf)
    mov eax, [esp + 4]   ; Load code_seg into EAX
    push eax             ; Push code_seg (as dword for 32-bit)
    push dword .next     ; Push the address of the next instruction
    retf                 ; Far return jumps to code_seg:.next

.next:
    ret                  ; Return to the calling function (now in new CS)


