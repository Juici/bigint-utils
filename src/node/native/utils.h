#include <stddef.h>
#include <stdint.h>

#if _MSC_VER
  #define inline __inline
#endif

#if defined(__GNUC__) || defined(__clang__)
static inline size_t clzb(uint64_t n) {
    return ((size_t) (__builtin_clzll(n) - ((sizeof(unsigned long long) * 8) - 64))) / 8;
}
#elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
  #include <intrin.h>

  #if defined(__x86_64__) || defined(__arm__) || defined(__aarch64__)
    #pragma intrinsic(_BitScanReverse64)

static inline size_t clzb(uint64_t n) {
    unsigned long index;
    _BitScanReverse64(&index, n);
    return (63 - index) / 8;
}
  #else
    #include <limits.h>
    #pragma intrinsic(_BitScanReverse)

static inline size_t clzb(uint64_t n) {
    unsigned long index;

    #if ULONG_MAX != UINT64_MAX
    if (n >> 32) {
        _BitScanReverse(&index, n >> 32);
        return (95 - index) / 8;
    } else {
    #endif
        _BitScanReverse(&index, n);
        return (63 - index) / 8;
    #if ULONG_MAX != UINT64_MAX
    }
    #endif
}
  #endif
#else
static inline size_t clzb(uint64_t n) {
    uint64_t a, b, r;
    if ((a = word >> 32)) {
        if ((b = a >> 16)) {
            r = (b >> 8) ? 0 : 1;
        } else {
            r = (a >> 8) ? 2 : 3;
        }
    } else {
        if ((a = word >> 16)) {
            r = (a >> 8) ? 4 : 5;
        } else {
            r = (word >> 8) ? 6 : 7;
        }
    }
    return r;
}
#endif

/**
 * Hint that there is no aliased memory between `a` and `b` for `n` bytes.
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