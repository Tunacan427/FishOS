section .text

global __flush_gdt
__flush_gdt:
    lgdt [rdi]
    mov ax, 0x10 ; ring 0 data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    pop rdi
    mov rax, 0x8 ; ring 0 code
    push rax
    push rdi
    retfq
