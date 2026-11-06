;
; StellaOS - gdt_flush.asm
; Reloads GDT register and segment registers
;

[BITS 32]
section .text
global gdt_flush

gdt_flush:
    mov eax, [esp + 4]      ; Get pointer to gdt_ptr_t
    lgdt [eax]              ; Load GDT

    mov ax, 0x10            ; 0x10 is the offset in the GDT to our kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:.flush         ; 0x08 is the offset to our kernel code segment: far jump
.flush:
    ret
