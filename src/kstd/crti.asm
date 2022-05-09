section .init
global _init:function
_init:
    push rbp
    mov rbp, rsp
    ; crtbegin.o .init

section .fini
global _fini:function
_fini:
    push rbp
    mov rbp, rsp
    ; crtbegin.o .fini
