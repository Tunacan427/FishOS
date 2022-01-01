section .text

global __flush_gdt
__flush_gdt:
    lgdt [rdi]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    pop rdi
    mov rax, 0x8
    push rax
    push rdi
    retfq
