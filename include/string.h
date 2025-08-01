#ifndef STRING_COMMANDS
#define STRING_COMMANDS

#include <stddef.h>
#include <stdint.h>

size_t strlen(const char* str);
int strcmp(const char* p1, const char* p2);
void * memmove (void *dest, const void *src, size_t len);
void *memcpy (void *dest, const void *src, size_t len);
char *strcat (char *dest, const char *src);
char* strcpy(char* __restrict dst, const char* __restrict src);
void* memset (void *dest, register int val, register size_t len);
int str_to_int(char* val);
uint64_t merge_bytes(uint8_t* bytes, int length);
void to_uppercase(char* s); 
int strncmp(const char *s1, const char *s2, register size_t n);
size_t STRLEN (const char *str);
#endif
