#ifndef UTILS_H
#define UTILS_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#ifdef _MSC_VER
    #define inline __inline
#endif

#if ULONG_MAX == UINT64_MAX
    #define ULONG_IS_64BIT 1
#endif

#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) ||                   \
    defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__)
    #define __IS_X86_64 1
#endif

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
    #include <intrin.h>
    #pragma intrinsic(_BitScanReverse)
    #define __has_BitScanReverse 1
    #if __IS_X86_64 || defined(__arm__) || defined(__aarch64__)
        #pragma intrinsic(_BitScanReverse64)
        #define __has_BitScanReverse64 1
    #endif
#endif

#if defined(__GNUC__) || defined(__clang__) || defined(__MINGW32__) || defined(__CYGWIN__)
    #if ULONG_IS_64BIT
static inline size_t clzb(uint64_t n) { return ((size_t) __builtin_clzl((unsigned long) n)) / 8; }
    #else
static inline size_t clzb(uint64_t n) {
    return ((size_t) __builtin_clzll((unsigned long long) n)) / 8;
}
    #endif
#elif __has_BitScanReverse || defined(__ICL) || defined(__INTEL_COMPILER)
    #if __has_BitScanReverse64
static inline size_t clzb(uint64_t n) {
    unsigned long index;
    _BitScanReverse64(&index, n);
    return (63 - ((size_t) index)) / 8;
}
    #else

static inline size_t clzb(uint64_t n) {
    unsigned long index;

        #if !ULONG_IS_64BIT
    if (n >> 32) {
        _BitScanReverse(&index, n >> 32);
        return (95 - ((size_t) index)) / 8;
    }
        #endif

    _BitScanReverse(&index, n);
    return (63 - ((size_t) index)) / 8;
}
    #endif
#else
static inline size_t clzb(uint64_t n) {
    uint64_t a, b, r;
    if ((a = n >> 32)) {
        if ((b = a >> 16)) {
            r = (b >> 8) ? 0 : 1;
        } else {
            r = (a >> 8) ? 2 : 3;
        }
    } else {
        if ((a = n >> 16)) {
            r = (a >> 8) ? 4 : 5;
        } else {
            r = (n >> 8) ? 6 : 7;
        }
    }
    return (size_t) r;
}
#endif

/**
 * Hint that there is no aliased memory between `a` and `b`.
 */
static inline void revswap(uint8_t *restrict a, uint8_t *restrict b, size_t n) {
    uint8_t x, y;
    for (size_t i = 0; i < n; i++) {
        x = a[i];
        y = b[n - 1 - i];

        a[i] = y;
        b[n - 1 - i] = x;
    }
}

static inline void reverse(uint8_t *data, size_t len) {
    size_t half_len = len / 2;

    uint8_t *front_half = data;
    uint8_t *back_half = &data[len - half_len];

    revswap(front_half, back_half, half_len);
}

#endif // UTILS_H