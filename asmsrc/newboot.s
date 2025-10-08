
section .text
org 0x9000
[BITS 16]
    cli
    call load_mem_map
    pushad ;this is necessary for some reason. vbe_set_mode clobbers an important register apparently and tbh id rather just do this than try to figure out which
    mov ax, 1280 ;width
    mov bx, 1024 ;height
    mov cl, 32 ;bits per pixel
    call vbe_set_mode
    cli
    popad 
    lgdt [gdt_descriptor]
    mov eax,cr0
    or eax,1 ;enable protected mode bit of control register
    mov cr0,eax 
    jmp 0x08:protected_mode_entry ;0x08 = code segment

load_mem_map:
    pushad ;something about this function screwed up the register state so i added this
    mov bx, 0x1000
    mov es, bx
    xor ebx, ebx ;bios loads map at es:di
    mov di, 4
    mov dword [es:0], 0 ;num of entries map
load_entry:
    mov eax, 0xE820 ; setup for bios call
    mov ecx, 24
    mov edx, 0x534D4150
    int 0x15 ; query bios for memory map
    jc end_map ;carry called when unsuccessful/end of list
    cmp ebx, 0; could also set ebx to 0 at end of list
    je end_map
    mov eax, [es:0]
    inc eax
    mov [es:0], eax
    add di, 24
    jmp load_entry
end_map:
    popad
    ret

[BITS 32]
protected_mode_entry:
    mov ax, 0x10 ; data segment selector
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x90000
    mov ebp, esp
    mov edi, 1
    mov si, 1
    mov ebx, 0x100000
    call read_sectors
    mov ecx, [0x100050] ;number of partitions
    mov eax, ecx
    shr eax, 2
    add eax, 1 
    mov edi, [0x100048] ;lba of partition entries
    mov cx, [0x10004C] ;lba of partition entries
    mov si, ax
    mov ebx, 0x100000
    call read_sectors
    xor edi, edi

load_kernel:
    mov si, 1 ;first, we read bpb to get fs info
    mov ebx, 0x200000
    mov edi, [0x7E00]
    mov cx, [0x7E04]
    call read_sectors
    mov bx, [0x200000+14] ;reserved sectors
    mov [0x7E08], bx ;save for later
    mov ax, [0x200000+16] ;num of fats
    movzx eax, ax ;make sure top bytes are zeroed
    mov ecx, [0x200000+36] ;sectors per fat
    mov dl, [0x200000+13] ;sectors per cluster
    mov [0x7E0A], dl 
    movzx si, dl
    mul ecx ;eax is implicit
    movzx ebx, bx
    add eax, ebx
    mov [0x7E0C], eax ;first data sector offset, starts from cluster 2
    mov edi, [0x7E00] ;read_sectors shouldnt modify edi but just in case
    mov cx, [0x7E04]
    add edi, eax
    adc cx, 0 ;carry over to cx if necessary
    mov [0x7E10], edi
    mov [0x7E14], cx
    mov ebx, 0x200000
    call read_sectors
    xor ecx, ecx
    movzx esi, si
    shl esi, 4 ;16 entries per sector
    call searchloop
    
    


notfound:
    mov word[0xB8000], 'K' | 0x0F00
    mov word[0xB8002], 'E' | 0x0F00
    mov word[0xB8004], 'R' | 0x0F00
    mov word[0xB8006], 'N' | 0x0F00
    mov word[0xB8008], 'E' | 0x0F00
    mov word[0xB800A], 'L' | 0x0F00
    mov word[0xB800C], ' ' | 0x0F00
    mov word[0xB800E], 'N' | 0x0F00
    mov word[0xB8010], 'O' | 0x0F00
    mov word[0xB8012], 'T' | 0x0F00
    mov word[0xB8014], ' ' | 0x0F00
    mov word[0xB8016], 'F' | 0x0F00
    mov word[0xB8018], 'O' | 0x0F00
    mov word[0xB801A], 'U' | 0x0F00
    mov word[0xB801C], 'N' | 0x0F00
    mov word[0xB801E], 'D' | 0x0F00
    mov word[0xB8020], ' ' | 0x0F00
    mov word[0xB8022], 'B' | 0x0F00
    mov word[0xB8024], 'O' | 0x0F00
    mov word[0xB8026], 'O' | 0x0F00
    mov word[0xB8028], 'T' | 0x0F00
    mov word[0xB802A], ' ' | 0x0F00
    mov word[0xB802C], 'H' | 0x0F00
    mov word[0xB802E], 'A' | 0x0F00
    mov word[0xB8030], 'L' | 0x0F00
    mov word[0xB8032], 'T' | 0x0F00
    mov word[0xB8034], 'E' | 0x0F00
    mov word[0xB8036], 'D' | 0x0F00
    hlt

gdt_start:
    dq 0x0000000000000000 ; null descriptor
    dq 0x00CF9A000000FFFF ; code segment, base=0, limit 4GB, executable+readable
    dq 0x00CF92000000FFFF ;data segment, base 0, limit 4GB, writable
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; size-1
    dd gdt_start ;offset

searchloop:
    xor edi, edi
    cmp ecx, esi
    je notfound
    mov eax, ecx
    shl eax, 5 ;mult by 32, size of file entry
    mov ebx, [0x200000+eax]
    cmp ebx, 0x4d475950 ;couldnt get string cmp to work so this is the raw ascii bytes, but it's just "PYGMALI KE" in reverse
    je check1
    end1:
    mov ebx, [0x200000+eax+4]
    cmp ebx, 0x20494c41
    je check2
    end2:
    mov bx, [0x200000+eax+8]
    cmp bx, 0x454b
    je check3
    end3:
    cmp edi, 3
    je kernelfound
    inc ecx
    jmp searchloop
    
kernelfound:
    xor ebx, ebx
    mov bx, [0x200000+eax+20] ;high cluster num
    shl ebx, 16
    mov bx, [0x200000+eax+26] ;low cluster num
    mov esi, [0x200000+eax+28] ;file size in bytes
    add esi, 511
    shr esi, 9 ;divide by 512 to get sectors
    mov [0x7E20], ebx ;cluster number stored here
    mov cl, [0x7E0A] ;cl now holds sectors per cluster
    mov eax, esi
    movzx ecx, cl
    xor edx, edx
    div ecx ;eax now has number of clusters for kernel
    add eax, 1
    mov [0x7E24], eax
    mov dword [0x7E1A], 0 ;memory offset for loading kernel
    xor eax, eax
    jmp loadloop
    
   
loadloop:
    movzx ebx, byte [0x7E0A] ;sectors per cluster
    mul ebx
    shl eax, 9
    add eax, 0x200000
    mov esi, eax
    mov eax, [0x7E20] ;cluster number
    movzx ebx, byte [0x7E0A] ;sectors per cluster
    movzx ecx, word [0x7E14] ;high data lba
    mov edi, [0x7E10] ;low data lba
    sub eax, 2
    mul ebx ;overflow from mul goes in edx
    mov ebx, esi
    add edi, eax
    adc ecx, edx
    movzx esi, byte [0x7E0A] ;sectors per cluster
    call read_sectors
    mov ecx, [0x7E20] ;cluster number
    shl ecx, 2
    movzx ebx, word [0x7E08] ;reserved sectors, beginning of fat
    mov eax, [0x7E00] ;low partition lba
    mov edx, [0x7E04] ;hi partition lba
    add eax, ebx
    adc edx, 0
    mov edi, ecx 
    and edi, 511 ;modulus 512
    mov [0x7E16], edi ;byte offset in fat
    shr ecx, 9
    add eax, ecx
    adc edx, 0
    mov cx, dx
    mov edi, eax
    mov si, 1
    mov ebx, 0x8000
    call read_sectors
    mov eax, [0x7E16]
    mov ebx, [0x8000+eax]
    cmp ebx, 0x0FFFFFF8 ;end of file
    jae finload
    cmp ebx, 0
    je notfound
    mov [0x7E20], ebx
    mov eax, [0x7E1A]
    inc eax
    mov [0x7E1A], eax
    jmp loadloop


finload:
    mov edi, [0x200038] ;program header offset
    mov eax, [0x7E1A]
    inc eax ;number of loaded clusters
    movzx ebx, byte [0x7E0A] ;sectors per cluster
    mul ebx
    shl eax, 9 
    xor ecx, ecx
    mov ebx, [0x200018]
shiftloop:
    mov esi, 0x200000
    add esi, ecx
    mov edx, [esi]
    sub esi, edi
    mov [esi], edx
    add ecx, 4
    cmp ecx, eax
    jl shiftloop
    jmp ebx

    
check1:
    inc edi
    jmp end1
check2:
    inc edi
    jmp end2
check3:
    inc edi
    jmp end3

read_sectors: ;cx:edi has lba, si has sector count, ebx has start memory address
    mov al, 0xE0 
    mov dx, 0x1F6
    out dx, al
    mov ax, si
    shr ax, 8 ;load top byte of sector count
    mov dx, 0x1F2
    out dx, al
    mov dx, 0x1F3
    mov eax, edi
    shr eax, 24 ;4th byte
    out dx, al
    mov dx, 0x1F4
    mov ax, cx ;5th byte
    out dx, al
    mov dx, 0x1F5
    shr ax, 8 ;get 6th byte
    out dx, al
    mov dx, 0x1F2
    mov ax, si
    out dx, al
    mov dx, 0x1F3
    mov eax, edi
    out dx, al ;1st byte
    mov dx, 0x1F4
    shr eax, 8
    out dx, al ;2nd byte
    mov dx, 0x1F5
    shr eax, 8
    out dx, al ;3rd byte
    mov ecx, 0
    call busypoll
    mov dx, 0x1F7
    mov al, 0x24
    out dx, al
    call readypoll
    mov edi, ebx
    movzx ebx, si
    shl ebx, 8 ;shl 8 is multiplying by 256
    call readloop
    ret

readloop:
    cmp ecx, ebx
    je endloop
    call busypoll
    call readypoll
    mov dx, 0x1F0
    in ax, dx
    mov [edi+ ecx*2], ax
    inc ecx
    jmp readloop
endloop:
ret

busypoll:
    mov dx, 0x1F7
    in al, dx
    call waitloop
    and al, 0x80 ;check busy bit
    cmp al, 0
    jne busypoll
    ret

readypoll:
    mov dx, 0x1F7
    in al, dx
    call waitloop
    and al, 0x08
    cmp al, 0
    je readypoll
    ret
    
waitloop:
    mov ah, al
    mov dx, 0x80
    mov al, 0
    out dx, al
    mov al, ah
    ret

[BITS 16]
; this code taken/modified from https:;wiki.osdev.org/VESA_Video_Modes

vbe_set_mode:
	mov [.width], ax
	mov [.height], bx
	mov [.bpp], cl
    xor ax, ax
    mov es, ax

	sti

	push es					; some VESA BIOSes destroy ES, or so I read
	mov ax, 0x4F00				; get VBE BIOS info
	mov di, vbe_info_block
	int 0x10
	pop es

	cmp ax, 0x4F				; BIOS doesn't support VBE?
	jne .error

	mov ax, word[vbe_info_block+vbe_info_structure.video_modes]
	mov [.offset], ax
	mov ax, word[vbe_info_block+vbe_info_structure.video_modes+2]
	mov [.segment], ax

	mov ax, [.segment]
	mov fs, ax
	mov si, [.offset]

.find_mode:
	mov dx, [fs:si]
	add si, 2
	mov [.offset], si
	mov [.mode], dx
	mov ax, 0
	mov fs, ax

	cmp word [.mode], 0xFFFF			; end of list?
	je .error

	push es
	mov ax, 0x4F01				; get VBE mode info
	mov cx, [.mode]
	mov di, mode_info_block
	int 0x10
	pop es

	cmp ax, 0x4F
	jne .error

	mov ax, [.width]
	cmp ax, [mode_info_block+vbe_mode_info_structure.width]
	jne .next_mode

	mov ax, [.height]
	cmp ax, [mode_info_block+vbe_mode_info_structure.height]
	jne .next_mode

	mov al, [.bpp]
	cmp al, [mode_info_block+vbe_mode_info_structure.bpp]
	jne .next_mode

	; If we make it here, we've found the correct mode!
	; Set the mode
	push es
	mov ax, 0x4F02
	mov bx, [.mode]
	or bx, 0x4000			; enable LFB
	mov di, 0			; not sure if some BIOSes need this... anyway it doesn't hurt
	int 0x10
	pop es

	cmp ax, 0x4F
	jne .error

	clc
	ret

.next_mode:
	mov ax, [.segment]
	mov fs, ax
	mov si, [.offset]
	jmp .find_mode

.error:
	stc
	ret

.width				dw 0
.height				dw 0
.bpp				db 0
.segment			dw 0
.offset				dw 0
.mode				dw 0
struc vbe_info_structure
     .signature resd 1	; must be "VESA" to indicate valid VBE support
	 .version resw 1;			; VBE version; high byte is major version, low byte is minor version
	 .oem resd 1;			; segment:offset pointer to OEM
	.capabilities resd 1;		; bitfield that describes card capabilities
	.video_modes resd 1;		; segment:offset pointer to list of supported video modes
	 .video_memory resw 1;		; amount of video memory in 64KB blocks
	 .software_rev resw 1;		; software revision
	 .vendor resd 1;			; segment:offset to card vendor string
	 .product_name resd 1;		; segment:offset to card model name
	 .product_rev resd 1;		; segment:offset pointer to product revision
	 .reserved resb 222;		; reserved for future expansion
	 .oem_data resb 256		; OEM BIOSes store their strings in this area
endstruc
struc vbe_mode_info_structure 
	 .attributes resw 1;		; deprecated, only bit 7 should be of interest to you, and it indicates the mode supports a linear frame buffer.
	 .window_a resb 1;			; deprecated
	 .window_b resb 1;			; deprecated
	 .granularity resw 1;		; deprecated; used while calculating bank numbers
	 .window_size resw 1;
	 .segment_a resw 1;
	 .segment_b resw 1;
	 .win_func_ptr resd 1;		; deprecated; used to switch banks from protected mode without returning to real mode
	 .pitch resw 1;			; number of bytes per horizontal line
	 .width resw 1;			; width in pixels
	 .height resw 1;			; height in pixels
	 .w_char resb 1;			; unused...
	 .y_char resb 1;			; ...
	 .planes resb 1;
	 .bpp resb 1;			; bits per pixel in this mode
	 .banks resb 1;			; deprecated; total number of banks in this mode
	 .memory_model resb 1;
	 .bank_size resb 1;		; deprecated; size of a bank, almost always 64 KB but may be 16 KB...
	 .image_pages resb 1;
	 .reserved0 resb 1;

	 .red_mask resb 1;
	 .red_position resb 1;
	 .green_mask resb 1;
	 .green_position resb 1;
	 .blue_mask resb 1
	 .blue_position resb 1;
	 .reserved_mask resb 1;
	 .reserved_position resb 1;
	 .direct_color_attributes resb 1;

	 .framebuffer resd 1;		; physical address of the linear frame buffer; write here to draw to the screen
	 .off_screen_mem_off resd 1;
	 .off_screen_mem_size resw 1;	; size of memory in the framebuffer but not being displayed on the screen
	 .reserved1 resb 206;
endstruc

	mode_info_block equ 0xf000

	vbe_info_block equ 0xe000