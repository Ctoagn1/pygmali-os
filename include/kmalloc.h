#define ALIGN4(x) (((x) + 3) & ~3)
#define ALIGN8(x) (((x) + 7) & ~7)
#define ALIGN16(x) (((x) + 15) & ~15)
#define MIN_BLOCK_SIZE 4
#include <stdint.h>
#include <stddef.h>
extern void* heap_start; 
extern void* heap_end;
extern char _end;
void* kmalloc(size_t size);
void kfree(void* to_be_freed);
void* krealloc(void* ptr, size_t size);
