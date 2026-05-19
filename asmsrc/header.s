section .multiboot
ALIGN  8

header_start:
    dd 0xE85250D6          ; magic 
    dd  0                      ; architecture (i386) 
    dd  header_end - header_start
    dd  -(0xE85250D6 + 0 + header_end - header_start)

framebuffer_tag:
    dw 5 ;type = framebuffer
    dw 0
    dd 20
    dd 1280 ;width
    dd 1024 ;height
    dd 32 ;depth
framebuffer_tag_end:
ALIGN 8
end_tag:
    dw 0
    dw 0
    dd 8
header_end: