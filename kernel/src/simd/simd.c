#include <stdint.h>
#include <stddef.h>
#include "../include/simd/simd.h"
#include "../include/drivers/serial.h"
#include "../libc/string.h"
#include "../libc/stdio.h"

void cpuid(uint32_t eax, uint32_t ecx, uint32_t *eax_out, uint32_t *ebx_out, 
           uint32_t *ecx_out, uint32_t *edx_out) {
    asm volatile (
        "cpuid"
        : "=a"(*eax_out), "=b"(*ebx_out), "=c"(*ecx_out), "=d"(*edx_out)
        : "a"(eax), "c"(ecx)
    );
}

uint32_t cpuid_get_max_leaf(void) {
    uint32_t eax, ebx, ecx, edx;
    cpuid(0, 0, &eax, &ebx, &ecx, &edx);
    return eax;
}

const char* get_cpu_vendor(void) {
    static char vendor[13] = {0};
    static int initialized = 0;
    
    if (!initialized) {
        uint32_t eax, ebx, ecx, edx;
        cpuid(0, 0, &eax, &ebx, &ecx, &edx);
        
        *((uint32_t*)&vendor[0]) = ebx;
        *((uint32_t*)&vendor[4]) = edx;
        *((uint32_t*)&vendor[8]) = ecx;
        vendor[12] = '\0';
        
        initialized = 1;
    }
    
    return vendor;
}

void get_cpu_brand_string(char *brand) {
    uint32_t eax, ebx, ecx, edx;
    
    cpuid(0x80000000, 0, &eax, &ebx, &ecx, &edx);
    if (eax < 0x80000004) {
        strcpy(brand, "Brand string not supported");
        return;
    }
    
    char *ptr = brand;
    for (uint32_t i = 0x80000002; i <= 0x80000004; i++) {
        cpuid(i, 0, &eax, &ebx, &ecx, &edx);
        *((uint32_t*)ptr) = eax;
        *((uint32_t*)(ptr + 4)) = ebx;
        *((uint32_t*)(ptr + 8)) = ecx;
        *((uint32_t*)(ptr + 12)) = edx;
        ptr += 16;
    }
    *ptr = '\0';
    
    char *src = brand, *dst = brand;
    int prev_space = 0;
    while (*src) {
        if (*src == ' ') {
            if (!prev_space && dst != brand) {
                *dst++ = ' ';
            }
            prev_space = 1;
        } else {
            *dst++ = *src;
            prev_space = 0;
        }
        src++;
    }
    *dst = '\0';
}

int check_sse_support(void) {
    uint32_t eax, ebx, ecx, edx;
    
    if (cpuid_get_max_leaf() < 1) {
        return 0;
    }
    
    cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    return (edx & (1 << 25)) != 0; 
}

int check_sse2_support(void) {
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    return (edx & (1 << 26)) != 0; 
}

int check_sse3_support(void) {
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    return (ecx & (1 << 0)) != 0; 
}

int check_ssse3_support(void) {
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    return (ecx & (1 << 9)) != 0; 
}

int check_sse4_1_support(void) {
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    return (ecx & (1 << 19)) != 0; 
}

int check_sse4_2_support(void) {
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    return (ecx & (1 << 20)) != 0; 
}

int check_avx_support(void) {
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    
    if ((ecx & (1 << 27)) && (ecx & (1 << 28))) {
        uint64_t xcr0;
        asm volatile ("xgetbv" : "=A"(xcr0) : "c"(0));
        return (xcr0 & 0x6) == 0x6; 
    }
    return 0;
}

int check_avx2_support(void) {
    uint32_t eax, ebx, ecx, edx;
    
    cpuid(0x80000000, 0, &eax, &ebx, &ecx, &edx);
    if (eax < 7) {
        return 0;
    }
    
    cpuid(7, 0, &eax, &ebx, &ecx, &edx);
    return check_avx_support() && (ebx & (1 << 5)); 
}

simd_level_t detect_simd_capabilities(void) {
    if (check_avx2_support()) return SIMD_AVX2;
    if (check_avx_support()) return SIMD_AVX;
    if (check_sse4_2_support()) return SIMD_SSE4_2;
    if (check_sse4_1_support()) return SIMD_SSE4_1;
    if (check_ssse3_support()) return SIMD_SSSE3;
    if (check_sse3_support()) return SIMD_SSE3;
    if (check_sse2_support()) return SIMD_SSE2;
    if (check_sse_support()) return SIMD_SSE;
    return SIMD_NONE;
}

const char* simd_level_to_string(simd_level_t level) {
    switch (level) {
        case SIMD_NONE: return "None";
        case SIMD_MMX: return "MMX";
        case SIMD_SSE: return "SSE";
        case SIMD_SSE2: return "SSE2";
        case SIMD_SSE3: return "SSE3";
        case SIMD_SSSE3: return "SSSE3";
        case SIMD_SSE4_1: return "SSE4.1";
        case SIMD_SSE4_2: return "SSE4.2";
        case SIMD_AVX: return "AVX";
        case SIMD_AVX2: return "AVX2";
        default: return "Unknown";
    }
}

void print_cpu_features(void) {
    char brand[49];
    get_cpu_brand_string(brand);
    
    printf("\n=== CPU INFORMATION ===\n");
    printf("Vendor: ");
    printf(get_cpu_vendor());
    printf("\nBrand: ");
    printf(brand);
    printf("\n");
    
    printf("Max CPUID leaf: 0x");
    char buffer[16];
    printf(itoa(cpuid_get_max_leaf(), buffer, 16));
    printf("\n");
    
    printf("SIMD Support: ");
    simd_level_t level = detect_simd_capabilities();
    printf(simd_level_to_string(level));
    printf("\n");
    
    printf("Features: ");
    if (check_sse_support()) printf("SSE ");
    if (check_sse2_support()) printf("SSE2 ");
    if (check_sse3_support()) printf("SSE3 ");
    if (check_ssse3_support()) printf("SSSE3 ");
    if (check_sse4_1_support()) printf("SSE4.1 ");
    if (check_sse4_2_support()) printf("SSE4.2 ");
    if (check_avx_support()) printf("AVX ");
    if (check_avx2_support()) printf("AVX2 ");
    printf("\n======================\n");
}

int enable_sse_safe(void) {
    if (!check_sse_support()) {
        serial_puts("[SIMD] ERROR: SSE not supported by CPU!\n");
        return -1;
    }
    
    enable_sse(); 
    
    serial_puts("[SIMD] SSE enabled successfully\n");
    return 0;
}

void enable_sse(void) {
    uint64_t cr0, cr4;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1ULL << 2);
    cr0 |=  (1ULL << 1);
    asm volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");

    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1ULL << 9);
    cr4 |= (1ULL << 10);
    asm volatile("mov %0, %%cr4" :: "r"(cr4) : "memory");

    unsigned int mxcsr = 0x1F80;
    asm volatile("ldmxcsr %0" :: "m"(mxcsr));
}

void *memcpy_simd(void *restrict dst, const void *restrict src, size_t n) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;

    if (n < 32) {
        for (size_t i = 0; i < n; ++i) d[i] = s[i];
        return dst;
    }

    while (n && ((uintptr_t)d & 0x0F)) {
        *d++ = *s++;
        --n;
    }
    size_t chunks = n / 16;
    for (size_t i = 0; i < chunks; ++i) {
        asm volatile(
            "movdqu (%0), %%xmm0\n\t"
            "movdqu %%xmm0, (%1)\n\t"
            :
            : "r"(s), "r"(d)
            : "xmm0", "memory"
        );
        s += 16;
        d += 16;
    }

    size_t tail = n % 16;
    for (size_t i = 0; i < tail; ++i) *d++ = *s++;

    return dst;
}

void *memset_simd(void *s_ptr, int c, size_t n) {
    uint8_t *d = (uint8_t *)s_ptr;
    uint8_t val = (uint8_t)c;

    if (n == 0) return s_ptr;
    if (n < 32) {
        for (size_t i = 0; i < n; ++i) d[i] = val;
        return s_ptr;
    }

    while (n && ((uintptr_t)d & 0x0F)) {
        *d++ = val;
        --n;
    }

    uint8_t pattern[16];
    for (int i = 0; i < 16; ++i) pattern[i] = val;

    size_t chunks = n / 16;
    for (size_t i = 0; i < chunks; ++i) {
        asm volatile(
            "movdqu (%0), %%xmm0\n\t"
            "movdqu %%xmm0, (%1)\n\t"
            :
            : "r"(pattern), "r"(d)
            : "xmm0", "memory"
        );
        d += 16;
    }

    size_t tail = n % 16;
    for (size_t i = 0; i < tail; ++i) *d++ = val;

    return s_ptr;
}

void fxsave_area_save(void *fx_area) {
    asm volatile("fxsave (%0)" :: "r"(fx_area) : "memory");
}
void fxsave_area_restore(void *fx_area) {
    asm volatile("fxrstor (%0)" :: "r"(fx_area) : "memory");
}