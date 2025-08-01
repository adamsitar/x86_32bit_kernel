extern interrupt_handler

%macro no_error_code_interrupt_handler 1
global interrupt_handler_%1
interrupt_handler_%1:
  push dword 0 ; push 0 as error code
  push dword %1 ; push the interrupt number
  jmp common_interrupt_handler ; jump to the common handler
%endmacro

%macro error_code_interrupt_handler 1
global interrupt_handler_%1
  interrupt_handler_%1:
  push dword %1 ; push the interrupt number
  jmp common_interrupt_handler ; jump to the common handler
%endmacro

common_interrupt_handler: ; the common parts of the generic interrupt handler
  ; Save all registers first
  push eax
  push ebx
  push ecx
  push edx
  push esi
  push edi
  push ebp
  
  ; NOW calculate what ESP was before we started pushing
  mov eax, esp
  add eax, 28        ; 7 pushes × 4 bytes = 28 bytes
  push eax           ; Push the ORIGINAL ESP value
  
  call interrupt_handler
  
  add esp, 4         ; Skip the ESP value
  pop ebp
  pop edi
  pop esi
  pop edx
  pop ecx
  pop ebx
  pop eax
  ; return to the code that got interrupted
  iret

no_error_code_interrupt_handler 0 ; create handler for interrupt 0
no_error_code_interrupt_handler 1 ; create handler for interrupt 1
; .
; .
; .
error_code_interrupt_handler 7 ; create handler for interrupt 7

global load_idt
; load_idt - Loads the interrupt descriptor table (IDT).
; stack: [esp + 4] the address of the first entry in the IDT
; [esp ] the return address
load_idt:
  mov eax, [esp+4] ; load the address of the IDT into register eax
  lidt eax ; load the IDT
  ret ; return to the calling function
