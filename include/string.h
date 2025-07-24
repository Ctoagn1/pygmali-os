#ifndef STRING_COMMANDS
#define STRING_COMMANDS

#include <stddef.h>

size_t strlen(const char* str);
int strcmp(const char* p1, const char* p2);
void * memmove (void *dest, const void *src, size_t len);
void *memcpy (void *dest, const void *src, size_t len);
#endif
