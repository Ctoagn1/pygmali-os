global gdtr
global setGDT
global reloadSegments
global reloadTSS
global reloadIDT
[BITS 32]
section .data
gdtr:
   dw 0 ; For limit storage 2 bytes 
   dd 0 ; For base storage 4 bytes 
idtr:
   dd 0 ; For limit storage 2 bytes 
   dw 0 ;For base storage 4 bytes 

section .text
setGDT:
   MOV   AX, [esp + 4]
   MOV   [gdtr], AX
   MOV   EAX, [ESP + 8]
   MOV   [gdtr + 2], EAX
   LGDT  [gdtr]
   RET
reloadSegments:
   ; Reload CS register containing code selector: 
   JMP   0x08:.reload_CS  ; 0x08 is kernel code in gdt
.reload_CS:
   ; Reload data segment registers 
   MOV   AX, 0x10 ; 0x10 is kernel data 
   MOV   DS, AX
   MOV   ES, AX
   MOV   FS, AX
   MOV   GS, AX
   MOV   SS, AX
   RET
reloadTSS:
   MOV AX, 0x28  ; location of tss in gdt 
   LTR AX
   RET
reloadIDT:
   MOV AX, [ESP+4]
   MOV [idtr], AX
   MOV EAX, [ESP + 8]
   MOV [idtr+ 2], EAX
   LIDT [idtr]
   ;STI
   RET

