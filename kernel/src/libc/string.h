#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
char* itoa(uint64_t value, char* str, int base);
size_t strlen(const char *str);
char* strcat(char* dest, const char* src);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
int strncmp(const char *s1, const char *s2, size_t n); 
int strcmp(const char *str1, const char *str2);

#endif // STRING_H