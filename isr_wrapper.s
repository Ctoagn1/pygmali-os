.intel_syntax noprefix
.globl   write_to_buffer_wrapper
.globl   update_time_wrapper
.globl   pit_timer_wrapper
.align   4
.globl exception_0_wrapper
.globl exception_1_wrapper
.globl exception_2_wrapper
.globl exception_3_wrapper
.globl exception_4_wrapper
.globl exception_5_wrapper
.globl exception_6_wrapper
.globl exception_7_wrapper
.globl exception_8_wrapper
.globl exception_9_wrapper
.globl exception_10_wrapper
.globl exception_11_wrapper
.globl exception_12_wrapper
.globl exception_13_wrapper
.globl exception_14_wrapper
.globl exception_16_wrapper
.globl exception_17_wrapper
.globl exception_18_wrapper
.globl exception_19_wrapper
.globl exception_20_wrapper
.globl exception_21_wrapper

write_to_buffer_wrapper:
    pushad
    cld     #C code following the sysV ABI requires DF to be clear on function entry
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
    push 14
    push esp
    cld
    call exception_handler
    popad
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