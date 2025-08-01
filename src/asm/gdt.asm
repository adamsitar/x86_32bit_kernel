global load_gdt ; make the label load_gdt visible outside this file
; load_gdt - load the Global Descriptor Table (GDT)
; stack: [esp + 4] the address of the GDT struct (limit:2 bytes, base:4 bytes)
; [esp] return address
load_gdt:
    mov eax, [esp + 4] ; move the address of the GDT struct into eax
    lgdt [eax]         ; load the GDT using the address in eax
    ret                ; return to the calling function


global flush_segments ; make the label flush_segments visible outside this file
; flush_segments - Load segment registers after GDT is set up
; This updates DS, ES, FS, GS, SS to the data segment, and does a far jump to update CS.
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
