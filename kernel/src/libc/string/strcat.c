#include <stddef.h>

char* strcat(char* dest, const char* src) {
    char* ptr = dest;
    
    // Находим конец dest
    while (*ptr != '\0') {
        ptr++;
    }
    
    // Копируем src в конец dest
    while (*src != '\0') {
        *ptr++ = *src++;
    }
    
    *ptr = '\0';
    return dest;
}