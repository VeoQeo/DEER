#include <stddef.h>

char* strncpy(char* dest, const char* src, size_t n) {
    if (dest == NULL || src == NULL) {
        return dest; 
    }

    char* ptr = dest;
    size_t i = 0;

    while (i < n && src[i] != '\0') {
        ptr[i] = src[i];
        i++;
    }

    while (i < n) {
        ptr[i] = '\0';
        i++;
    }

    return dest;
}