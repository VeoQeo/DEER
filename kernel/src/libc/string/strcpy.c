#include <stddef.h>

char* strcpy(char* dest, const char* src) {
    if (dest == NULL || src == NULL) {
        return NULL;
    }
    
    char* ptr = dest;
    while (*src != '\0') {
        *ptr++ = *src++;
    }
    *ptr = '\0';
    
    return dest;
}