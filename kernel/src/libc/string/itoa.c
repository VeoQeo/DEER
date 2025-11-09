#include <stdint.h>

char* itoa(uint64_t value, char* str, int base) {
    if (base < 2 || base > 16) {
        str[0] = '\0';
        return str;
    }

    char* ptr = str;
    char* end = str;

    if (value == 0) {
        *end++ = '0';
    } else {
        while (value > 0) {
            int digit = value % base;
            *end++ = (digit < 10 ? '0' + digit : 'a' + digit - 10);
            value /= base;
        }
    }

    *end = '\0';
    end--;
    while (ptr < end) {
        char tmp = *ptr;
        *ptr = *end;
        *end = tmp;
        ptr++;
        end--;
    }

    return str;
}