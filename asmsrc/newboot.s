
section .text
org 0x9000
[BITS 16]
    cli
    lgdt [gdt_descriptor]
    mov eax,cr0
    or eax,1 ;enable protected mode bit of control register
    mov cr0,eax 
    jmp 0x08:protected_mode_entry ;0x08 = code segment

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
