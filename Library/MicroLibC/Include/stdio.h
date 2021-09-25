#ifndef _STDIO_H
#define _STDIO_H

#include <stddef.h>
#include <stdarg.h>

int vsnprintf(char *str, size_t size, const char *format, va_list ap);
int snprintf(char *str, size_t size, const char *format, ...);

int vprintf(const char *format, va_list ap);
int printf(const char *format, ...);
int __printf_chk(const char *format, ...);

VOID *malloc(UINTN size);
VOID *malloc_rt(UINTN Size);
VOID *memalign(UINTN boundary, UINTN size);
VOID *memalign2(UINTN boundary, UINTN size, BOOLEAN runtime);
VOID *calloc(UINTN count, UINTN size);
VOID  free(VOID *ptr);

#endif
