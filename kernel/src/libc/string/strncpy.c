#include <stddef.h>

char* strncpy(char* dest, const char* src, size_t n) {
    if (dest == NULL || src == NULL) {
        return dest; // Неопределенное поведение по стандарту, но возвращаем dest
    }

    char* ptr = dest;
    size_t i = 0;

    // Копируем символы из src в dest, пока не достигнем конца src или n символов
    while (i < n && src[i] != '\0') {
        ptr[i] = src[i];
        i++;
    }

    // Заполняем оставшиеся позиции в dest нулевыми байтами
    while (i < n) {
        ptr[i] = '\0';
        i++;
    }

    return dest;
}