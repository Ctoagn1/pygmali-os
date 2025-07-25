.intel_syntax noprefix
.globl   write_to_buffer_wrapper
.globl   update_time_wrapper
.align   4

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
