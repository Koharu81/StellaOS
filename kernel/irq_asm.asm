;
; StellaOS - irq_asm.asm
; Low-level hardware IRQ stubs (IRQ 0-15 mapped to ISR 32-47)
;

[BITS 32]
section .text

extern irq_handler

%macro IRQ 2
global irq%1
irq%1:
    cli
    push byte 0         ; Dummy error code
    push byte %2        ; Interrupt vector number (32 + %1)
    jmp irq_common_stub
%endmacro

IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

irq_common_stub:
    pusha               ; Pushes edi, esi, ebp, esp, ebx, edx, ecx, eax

    mov ax, ds          ; Lower 16-bits of eax = ds
    push eax            ; Save the data segment descriptor

    mov ax, 0x10        ; Load the kernel data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp            ; Push pointer to registers_t as argument to irq_handler
    call irq_handler
    add esp, 4          ; Clean up pushed pointer argument

    pop eax             ; Reload original data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa                ; Pops edi, esi, ebp, esp, ebx, edx, ecx, eax
    add esp, 8          ; Clean up pushed error code and pushed IRQ number
    iret                ; Pops CS, EIP, EFLAGS, SS, and ESP
