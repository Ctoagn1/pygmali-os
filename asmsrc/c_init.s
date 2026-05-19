
global framebuffer_1
extern _bss_start
extern _bss_end
extern kernel_main
extern _stack_top
global _kernel_start

magic_num:
dd 0
multiboot_ptr:
dd 0

virtual_offset equ 0xc0000000
section .text

_kernel_start:
    mov [magic_num-virtual_offset], eax
    mov [multiboot_ptr-virtual_offset], ebx
    mov eax, _bss_start - virtual_offset
zero_bss:
    mov dword [eax], 0
    add eax, 4
    cmp eax, _bss_end - virtual_offset
    jl zero_bss ; relative jump, so no offset necessary
jmp setup

ALIGN 4096
page_dir:
dd temp_identity_map + 3 - virtual_offset
times 767 dd 0
dd high_page_table + 131 - virtual_offset
times 252 dd 0
dd framebuffer_1 + 131 - virtual_offset
dd framebuffer_2 + 131 - virtual_offset
dd page_dir + 3 - virtual_offset ;recursive mapping
high_page_table:
times 1024 dd 0
framebuffer_1:
times 1024 dd 0
framebuffer_2:
times 1024 dd 0
temp_identity_map:
times 1024 dd 0

setup:
xor ecx, ecx
kernel_map_setup:
mov eax, ecx
shl eax, 12
or eax, 3
mov [high_page_table-virtual_offset+ecx*4], eax
mov [temp_identity_map-virtual_offset+ecx*4], eax
inc ecx
cmp ecx, 1024
jne kernel_map_setup

init_paging:
mov esp, _stack_top
mov ebp, esp
mov eax, page_dir - virtual_offset
mov cr3, eax
mov eax, cr0
or eax, 0x80000000
mov cr0, eax
jmp high_mapping
mov eax, cr4
or eax, 0b10000000 ; enable global pages
high_mapping:
    mov eax, [magic_num]
    mov ebx, [multiboot_ptr]
    push ebx
    push eax
call kernel_main + virtual_offset
err:
    cli
    hlt
    jmp err


