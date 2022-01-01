%macro pushall 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro popall 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

section .text
extern __idt_handler_common
__idt_wrapper_common:
    pushall
    mov rdi, rsp
    call __idt_handler_common
    popall
    add rsp, 16
    iretq

%define has_errcode(v) (v == 8 || (v >= 10 && v <= 14) || v == 17 || v == 21)

; generate idt wrappers
%assign vector 0
%rep 256
__idt_wrapper_%+vector:
%if !has_errcode(vector)
    push qword 0
%endif
    push qword vector
    jmp __idt_wrapper_common
%assign vector vector+1
%endrep

section .data
; generate array of function pointers for the wrappers
global __idt_wrappers
__idt_wrappers:
%assign vector 0
%rep 256
    dq  __idt_wrapper_%+vector
%assign vector vector + 1
%endrep
