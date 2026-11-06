;
; StellaOS - boot.asm
; Multiboot2 entry point targeting i686 / 32-bit protected mode
;

[BITS 32]

; Multiboot2 constants
MULTIBOOT2_HEADER_MAGIC         equ 0xE85250D6
MULTIBOOT2_ARCHITECTURE_I386    equ 0
MULTIBOOT2_TAG_FRAMEBUFFER      equ 5
MULTIBOOT2_TAG_END              equ 0

section .multiboot
align 8
header_start:
    ; Magic number
    dd MULTIBOOT2_HEADER_MAGIC
    ; Architecture: i386
    dd MULTIBOOT2_ARCHITECTURE_I386
    ; Header length
    dd header_end - header_start
    ; Checksum
    dd -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT2_ARCHITECTURE_I386 + (header_end - header_start))

    ; Framebuffer tag request (1280x720x32)
    align 8
framebuffer_tag_start:
    dw MULTIBOOT2_TAG_FRAMEBUFFER   ; Type = 5
    dw 0                            ; Flags = 0
    dd 20                           ; Size = 20 bytes
    dd 1280                         ; Width = 1280
    dd 720                          ; Height = 720
    dd 32                           ; Depth = 32 bpp
framebuffer_tag_end:

    ; End tag
    align 8
    dw MULTIBOOT2_TAG_END           ; Type = 0
    dw 0                            ; Flags = 0
    dd 8                            ; Size = 8 bytes
header_end:

section .bss
align 16
stack_bottom:
    resb 16384                      ; 16 KB stack
stack_top:

section .text
global _start
extern kernel_main

_start:
    ; Disable interrupts
    cli
    cld

    ; Set up the stack pointer
    mov esp, stack_top

    ; Reset EFLAGS
    push 0
    popfd

    ; Multiboot2 bootloader specification passes:
    ; EAX = MULTIBOOT2_BOOTLOADER_MAGIC (0x36D76289)
    ; EBX = 32-bit physical address of the Multiboot2 information structure
    ; Push arguments for kernel_main(uint32_t magic, uint32_t mbi_addr)
    push ebx                        ; 2nd argument: multiboot info pointer
    push eax                        ; 1st argument: multiboot magic

    ; Transfer control to the C kernel
    call kernel_main

    ; If kernel_main returns, halt CPU in an infinite loop
    cli
.hang:
    hlt
    jmp .hang
