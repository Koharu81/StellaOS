;
; StellaOS - idt_load.asm
; Loads the Interrupt Descriptor Table (IDTR)
;

[BITS 32]
section .text
global idt_load

idt_load:
    mov eax, [esp + 4]      ; Get pointer to idt_ptr_t
    lidt [eax]              ; Load IDT
    ret
