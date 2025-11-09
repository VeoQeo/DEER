#ifndef SIMD_H
#define SIMD_H

#include <stddef.h>
#include <stdint.h>

void cpuid(uint32_t eax, uint32_t ecx, uint32_t *eax_out, uint32_t *ebx_out, 
           uint32_t *ecx_out, uint32_t *edx_out);
uint32_t cpuid_get_max_leaf(void);
const char* get_cpu_vendor(void);
void get_cpu_brand_string(char *brand);

int check_sse_support(void);
int check_sse2_support(void);
int check_sse3_support(void);
int check_ssse3_support(void);
int check_sse4_1_support(void);
int check_sse4_2_support(void);
int check_avx_support(void);
int check_avx2_support(void);

typedef enum {
    SIMD_NONE = 0,
    SIMD_MMX,
    SIMD_SSE,
    SIMD_SSE2,
    SIMD_SSE3,
    SIMD_SSSE3,
    SIMD_SSE4_1,
    SIMD_SSE4_2,
    SIMD_AVX,
    SIMD_AVX2
} simd_level_t;

simd_level_t detect_simd_capabilities(void);
const char* simd_level_to_string(simd_level_t level);
void print_cpu_features(void);

void enable_sse(void);
int enable_sse_safe(void);
void *memcpy_simd(void *restrict dst, const void *restrict src, size_t n);
void *memset_simd(void *s, int c, size_t n);
void fxsave_area_save(void *fx_area);
void fxsave_area_restore(void *fx_area);

#endif // SIMD_H 