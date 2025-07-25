#include <stddef.h>

size_t strlen(const char* str) 
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}
/* Public domain.  */
void * memmove (void *dest, const void *src, size_t len)
{
  char *d = dest;
  const char *s = src;
  if (d < s)
    while (len--)
      *d++ = *s++;
  else
    {
      char *lasts = s + (len-1);
      char *lastd = d + (len-1);
      while (len--)
        *lastd-- = *lasts--;
    }
  return dest;
}

void *
memcpy (void *dest, const void *src, size_t len)
{
  char *d = dest;
  const char *s = src;
  while (len--)
    *d++ = *s++;
  return dest;
}
char* strcpy(char* __restrict dst, const char* __restrict src)
{
	const size_t length = strlen(src);
	//  The stpcpy() and strcpy() functions copy the string src to dst
	//  (including the terminating '\0' character).
	memcpy(dst, src, length + 1);
	//  The strcpy() and strncpy() functions return dst.
	return dst;
}

#undef strcat

#ifndef STRCAT
# define STRCAT strcat
#endif

/* Append SRC on the end of DEST.  */
char *
strcat (char *dest, const char *src)
{
  strcpy (dest + strlen (dest), src);
  return dest;
}