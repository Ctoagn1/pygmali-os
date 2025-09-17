extern _bss_start
extern _bss_end
extern kernel_main
extern _stack_top
global _kernel_start

section .text
_kernel_start:
    mov eax, _bss_start
zero_bss:
    mov dword [eax], 0
    add eax, 4
    cmp eax, _bss_end
    jl zero_bss

xor eax, eax
xor ebx, ebx
xor ecx, ecx
xor edx, edx
mov esp, _stack_top
mov ebp, esp
call kernel_main
err:
    cli
    hlt
    jmp err