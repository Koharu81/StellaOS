;
; StellaOS - isr_asm.asm
; Low-level CPU exception stubs and common interrupt dispatcher
;

[BITS 32]
section .text

extern isr_handler

; Macro for ISRs that do not push an error code
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    cli
    push byte 0         ; Dummy error code
    push byte %1        ; Interrupt number
    jmp isr_common_stub
%endmacro

; Macro for ISRs that push an error code
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    cli
    ; Error code is already pushed by CPU
    push byte %1        ; Interrupt number
    jmp isr_common_stub
%endmacro

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_ERRCODE   21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_ERRCODE   30
ISR_NOERRCODE 31

isr_common_stub:
    pusha               ; Pushes edi, esi, ebp, esp, ebx, edx, ecx, eax

    mov ax, ds          ; Lower 16-bits of eax = ds
    push eax            ; Save the data segment descriptor

    mov ax, 0x10        ; Load the kernel data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp            ; Push pointer to registers_t as argument to isr_handler
    call isr_handler
    add esp, 4          ; Clean up pushed pointer argument

    pop eax             ; Reload original data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa                ; Pops edi, esi, ebp, esp, ebx, edx, ecx, eax
    add esp, 8          ; Clean up pushed error code and pushed ISR number
    iret                ; Pops CS, EIP, EFLAGS, SS, and ESP
