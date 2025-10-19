global   write_to_buffer_wrapper
global   update_time_wrapper
global   pit_timer_wrapper
extern write_to_buffer
extern update_time
extern pit_timer
extern exception_handler
extern syscall_handler
extern page_fault_handler
align   4
global exception_0_wrapper
global exception_1_wrapper
global exception_2_wrapper
global exception_3_wrapper
global exception_4_wrapper
global exception_5_wrapper
global exception_6_wrapper
global exception_7_wrapper
global exception_8_wrapper
global exception_9_wrapper
global exception_10_wrapper
global exception_11_wrapper
global exception_12_wrapper
global exception_13_wrapper
global exception_14_wrapper
global exception_16_wrapper
global exception_17_wrapper
global exception_18_wrapper
global exception_19_wrapper
global exception_20_wrapper
global exception_21_wrapper
global enable_paging
global syscall_handler_wrapper
[BITS 32]
enable_paging:
    push eax
    mov eax, [esp+8]
    mov cr3, eax
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    pop eax
    ret
write_to_buffer_wrapper:
    pushad
    cld     ;C code following the sysV ABI requires DF to be clear on function entry
    call write_to_buffer
    popad
    iret

update_time_wrapper:
    pushad
    cld     
    call update_time
    popad
    iret

pit_timer_wrapper:
    pushad
    cld     
    call pit_timer
    popad
    iret

exception_0_wrapper:
    pushad
    push 0
    push esp
    cld
    call exception_handler
    popad
    iret
exception_1_wrapper:
    pushad
    push 1
    push esp
    cld
    call exception_handler
    popad
    iret
exception_2_wrapper:
    pushad
    push 2
    push esp
    cld
    call exception_handler
    popad
    iret
exception_3_wrapper:
    pushad
    push 3
    push esp
    cld
    call exception_handler
    popad
    iret
exception_4_wrapper:
    pushad
    push 4
    push esp
    cld
    call exception_handler
    popad
    iret
exception_5_wrapper:
    pushad
    push 5
    push esp
    cld
    call exception_handler
    popad
    iret
exception_6_wrapper:
    pushad
    push 6
    push esp
    cld
    call exception_handler
    popad
    iret
exception_7_wrapper:
    pushad
    push 7
    push esp
    cld
    call exception_handler
    popad
    iret
exception_8_wrapper:
    pushad
    push 8
    push esp
    cld
    call exception_handler
    popad
    iret
exception_9_wrapper:
    pushad
    push 9
    push esp
    cld
    call exception_handler
    popad
    iret
exception_10_wrapper:
    pushad
    push 10
    push esp
    cld
    call exception_handler
    popad
    iret
exception_11_wrapper:
    pushad
    push 11
    push esp
    cld
    call exception_handler
    popad
    iret
exception_12_wrapper:
    pushad
    push 12
    push esp
    cld
    call exception_handler
    popad
    iret
exception_13_wrapper:
    pushad
    push 13
    push esp
    cld
    call exception_handler
    popad
    iret
exception_14_wrapper:
    pushad
    mov eax, [esp+32]
    push eax
    mov eax, cr2
    push eax
    cld
    call page_fault_handler
    add esp, 8 ;error code and page 
    popad
    add esp, 4
    iret
exception_16_wrapper:
    pushad
    push 16
    push esp
    cld
    call exception_handler
    popad
    iret
exception_17_wrapper:
    pushad
    push 17
    push esp
    cld
    call exception_handler
    popad
    iret
exception_18_wrapper:
    pushad
    push 18
    push esp
    cld
    call exception_handler
    popad
    iret
exception_19_wrapper:
    pushad
    push 19
    push esp
    cld
    call exception_handler
    popad
    iret
exception_20_wrapper:
    pushad
    push 20
    push esp
    cld
    call exception_handler
    popad
    iret
exception_21_wrapper:
    pushad
    push 21
    push esp
    cld
    call exception_handler
    popad
    iret
syscall_handler_wrapper:
    pushad
    call syscall_handler
    popad
    iret