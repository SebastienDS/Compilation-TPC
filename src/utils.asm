; utils.asm
section .data
    format_registers db "rbx:0x%lx r12:0x%lx r13:%ld r14:%ld", 10, 0
    format_stack db "sommet (rsp): 0x%lx, base du bloc (rbp): 0x%lx", 10, 0
    format_int db "%d", 10, 0

section .text
global show_registers
global show_stack
extern getchar
extern putchar
global getint
global putint
extern printf
show_registers:
    push rbp
    mov rbp, rsp
    
    mov r8,  r14
    mov rcx, r13
    mov rdx, r12
    mov rsi, rbx
    mov rdi, format_registers
    mov rax, 0
    call printf 
        
    pop rbp
    ret

show_stack:
    push rbp
    mov rbp, rsp

    mov rdx, [rsp]
    mov rax, rsp
    add rax, 16
    mov rsi, rax
    mov rdi, format_stack
    mov rax, 0
    call printf WRT ..plt
    
    pop rbp
    ret

; getchar:
;     push rbp
;     mov rbp, rsp

;     sub rsp, 8

;     mov rax, 0
;     mov rdi, 0
;     mov rsi, rsp
;     mov rdx, 1
;     syscall

;     pop rax

;     mov rsp, rbp
;     pop rbp
;     ret

; putchar:
;     push rbp
;     mov rbp, rsp

;     push rdi

;     mov rax, 1
;     mov rdi, 1
;     mov rsi, rsp
;     mov rdx, 1
;     syscall

;     pop rax

;     mov rsp, rbp
;     pop rbp
;     ret

getint:
    push rbp
    mov rbp, rsp

    sub rsp, 16
    ; number = rbp - 8
    ; digit = rbp - 16

    mov qword [rbp - 8], 0

    _getint_loop:
        call getchar
        mov qword [rbp - 16], rax

        cmp qword [rbp - 16], '0'
        jl _getint_ret
        cmp qword [rbp - 16], '9'
        jg _getint_ret

        sub qword [rbp - 16], '0'
        mov rax, qword [rbp - 8]
        imul rax, 10
        add rax, qword [rbp - 16]
        mov qword [rbp - 8], rax

        jmp _getint_loop

    _getint_ret:
        mov rax, qword [rbp - 8]

    mov rsp, rbp
    pop rbp
    ret

putint:
    push rbp
    mov rbp, rsp

    push rdi

    mov rdi, format_int
    pop rsi
    call printf 

    mov rsp, rbp
    pop rbp
    ret