# include <stdint.h>
#include "string.h"
#include "fd.h"
typedef struct elf_header{
    char magic[4];
    char class;
    char  data;
    char version;
    char abi;
    char abiv;
    char reserved[7];
    uint16_t type;
    uint16_t machine;
    uint32_t elf_version;
    uintptr_t entry;
    uintptr_t phoff;
    uintptr_t shoff;
    uint32_t flags;
    uint16_t hsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
} elf_header;
typedef struct program_header32{
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} program_header32;
typedef struct section_header32{
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;
} section_header32;

#define KERNEL_BASE 0xC0000000

int exec_elf(char* filepath){
    
}
int load_elf(void* elf, size_t elf_size){
    if(elf_size<sizeof(elf_header)) return -1;
    uint8_t* ptr = elf;
    elf_header* hdr = (elf_header*) ptr;
    if(hdr->magic[0]!=0x7F || strncmp(&hdr->magic[1], "ELF", 3)!=0) return -1;
    if(hdr->phoff==0) return -1;
    uint16_t entry_size=hdr->phentsize;
    uintptr_t entrypt = hdr->entry;
    uint16_t entry_num=hdr->phnum;
    uint32_t ph_table_size = hdr->phnum * hdr->phentsize;
    if(hdr->phoff + ph_table_size > elf_size) return -1;
    program_header32* phdr =(program_header32*)(ptr+hdr->phoff);
    for(int i=0; i<entry_num; i++){
        if(phdr->p_type == 1){ //1 = PT_LOAD
            if(phdr->p_filesz > phdr->p_memsz) return -1;
            if(phdr->p_offset + phdr->p_filesz>elf_size) return -1;
            if(phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr) return -1;
            if(phdr->p_vaddr >= KERNEL_BASE || phdr->p_vaddr+phdr->p_memsz>=KERNEL_BASE) return -1;

            memcpy((void*)phdr->p_vaddr, (ptr+phdr->p_offset), phdr->p_filesz);
            memset((void*)(phdr->p_vaddr+phdr->p_filesz), 0, phdr->p_memsz-phdr->p_filesz);
        }
        phdr+=1;
    }
    return 0;
}
