section .init
global _init
_init:
    push rbp
    mov rbp, rsp
    ; crtbegin.o .init

section .fini
global _fini
_fini:
    push rbp
    mov rbp, rsp
    ; crtbegin.o .fini
