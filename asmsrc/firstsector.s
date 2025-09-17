org 0x7c00 ;address that bios loads first sector at 
;i made this to search all partitions and load the right one, but realized that
;since this replaces the mbr there's no point in having multiple partitions
[BITS 16]
start:
	cli
	mov ax, 0
	mov ds, ax
	mov es, ax ;init segmaents to zero
	mov di, 0
	mov cx, 0
clear_video_memory:
	mov bx, 0xB800
	mov es, bx
	mov di, cx
	shl di, 1
	mov word [es:di], 0
	inc cx
	cmp cx, 2000
	jne clear_video_memory
	mov bx, 0
	mov es, bx
	in al, 0x92
	or al, 2
	out 0x92, al ;these 3 lines enable a20 line (though i think qemu enables it by default?)

	mov si, DAPACK		; address of "disk address packet"
	mov ah, 0x42		; AL is unused
	mov dl, 0x80		; drive number 0 (OR the drive ; with 0x80)
	int 0x13
	jc errmsg ;int 13h failure recorded in carry flag
	mov si, DAPACK
	mov ax, [0x7E50] ;number of partitions. only reading 2 bytes, god help you if you need 65536 partitions
	mov bx, ax
	shr ax, 2 ;divide by 4
	add ax, 1 
	mov word [si+2], ax
	mov ax, [0x7E48] ;lba of partition entries for low 2 bytes
	mov word [si+8], ax
	mov ax, [0x7E48+2]
	mov word [si+10], ax
	mov word [si+4], 0x8000
	mov ah, 0x42
	mov dl, 0x80
	int 0x13
	jc errmsg
	mov si, DAPACK
	mov cx, 0

partloop:
	cmp cx, bx
	je errmsg
	mov di, 0x8020
	mov ax, cx
	shl ax, 7 ;mult by 128
	add di, ax
	mov ax, [di]
	mov [si+8], ax
	mov ax, [di+2]
	mov [si+10], ax
	mov ax, [di+4]
	mov [si+12], ax
	mov ax, [di+6]
	mov [si+14], ax ;read bpb
	mov word [si+2], 1
	mov word [si+4], 0x7E00
	mov ah, 0x42
	mov dl, 0x80
	int 0x13
	jc errmsg
	mov si, DAPACK
	mov ax, [0x7E00+508]
	cmp ax, 0x07b0 ;signature
	je load
	inc cx
	jmp partloop
errmsg:
	mov ax, 'E'
	or ax, 0x0F00
	mov di, 0
	mov bx, 0xB800
	mov es, bx
	mov [es:di], ax
	mov ax, 'R'
	or ax, 0x0F00
	mov di, 2
	mov [es:di], ax
	mov di, 4
	mov [es:di], ax
	hlt
load:
	mov ax, [di] ;save lba of partition for later use
	mov [0x7E00], ax
	add ax, 1 ;sector after first
	mov [si+8], ax
	mov ax, [di+2]
	mov [0x7E02], ax
	mov ax, [di+4]
	mov [0x7E04], ax
	mov ax, [di+6]
	mov [0x7E06], ax
	mov ah, 0x42
	mov dl, 0x80
	mov word [si+2], 8 ;not sure about size yet
	mov word [si+6], 0 ;ensure segment is 0
	mov word [si+4], 0x9000
	int 0x13
	jc errmsg
	jmp 0x0000:0x9000

ALIGN 4
DAPACK: ;this data structure taken from osdev wiki
	db	0x10
	db	0
blkcnt:	dw	1	; int 13 resets this to # of blocks actually read/written
db_add:	dw	0x7E00	; memory buffer destination address (0:7c00)
	dw	0		; in memory page zero
d_lba:	dd	1	; put the lba to read in this spot
	dd	0		; more storage bytes only for big lba's ( > 4 bytes )

times 510-($-$$) db 0 ;pad to 512 bytes
dw 0xAA55