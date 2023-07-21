section .text

extern __idt_handler_common
__idt_wrapper_common:
    cmp QWORD [rsp + 16], 0x23 ; check if code segment is user
    jne .continue1
    swapgs

.continue1:
    xchg rax, [rsp] ; swap rax with the vector number which was pushed in the wrapper 
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

    mov rdi, rax ; put the vector number into the first parameter
    mov rsi, rsp ; put the interrupt frame pointer into the second parameter

    mov eax, es
    push rax
    mov eax, ds
    push rax
    
    sub rsi, 16

    cld

    mov eax, 0x10 ; kernel data segment
    mov ds, eax
    mov es, eax
    mov ss, eax

    xor rbp, rbp

    call __idt_handler_common

    pop rax
    mov ds, eax
    pop rax
    mov es, eax

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

    add rsp, 8 ; skip the error code

    cmp QWORD [rsp + 8], 0x23 ; check if code segment is user
    jne .continue2
    swapgs

.continue2:
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
