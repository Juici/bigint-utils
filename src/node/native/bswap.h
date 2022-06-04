#ifndef BSWAP_H
#define BSWAP_H

#include <stdint.h>

/* Linux */
#if defined(__linux__) || defined(__GLIBC__)
    #include <byteswap.h>
    #include <endian.h>
    #define __ENDIAN_DEFINED
    #define __BSWAP_DEFINED
    #define __HOSTSWAP_DEFINED
    #define _BYTE_ORDER __BYTE_ORDER
    #define _LITTLE_ENDIAN __LITTLE_ENDIAN
    #define _BIG_ENDIAN __BIG_ENDIAN
    #define bswap16(x) bswap_16(x)
    #define bswap32(x) bswap_32(x)
    #define bswap64(x) bswap_64(x)
#endif

/* BSD */
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__DragonFly__) || defined(__OpenBSD__)
    #include <sys/endian.h>
    #define __ENDIAN_DEFINED
    #define __BSWAP_DEFINED
    #define __HOSTSWAP_DEFINED
#endif

/* Apple */
#if defined(__APPLE__)
    #include <libkern/OSByteOrder.h>
    #include <machine/endian.h>
    #define __ENDIAN_DEFINED
    #define __BSWAP_DEFINED
    #define _BYTE_ORDER BYTE_ORDER
    #define _LITTLE_ENDIAN LITTLE_ENDIAN
    #define _BIG_ENDIAN BIG_ENDIAN
    #define bswap16(x) OSSwapInt16(x)
    #define bswap32(x) OSSwapInt32(x)
    #define bswap64(x) OSSwapInt64(x)
#endif

/* Windows */
#if defined(_WIN32) || defined(_MSC_VER)
    /* assumes all Microsoft targets are little endian */
    #include <stdlib.h>
    #define _LITTLE_ENDIAN 1234
    #define _BIG_ENDIAN 4321
    #define _BYTE_ORDER _LITTLE_ENDIAN
    #define __ENDIAN_DEFINED
    #define __BSWAP_DEFINED
    #define inline __inline
static inline uint16_t bswap16(uint16_t x) { return _byteswap_ushort(x); }
static inline uint32_t bswap32(uint32_t x) { return _byteswap_ulong(x); }
static inline uint64_t bswap64(uint64_t x) { return _byteswap_uint64(x); }
#endif

/* OpenCL */
#if defined(__OPENCL_VERSION__)
    #define _LITTLE_ENDIAN 1234
    #define _BIG_ENDIAN 4321
    #if defined(__ENDIAN_LITTLE__)
        #define _BYTE_ORDER _LITTLE_ENDIAN
    #else
        #define _BYTE_ORDER _BIG_ENDIAN
    #endif
    #define bswap16(x) as_ushort(as_uchar2(ushort(x)).s1s0)
    #define bswap32(x) as_uint(as_uchar4(uint(x)).s3s2s1s0)
    #define bswap64(x) as_ulong(as_uchar8(ulong(x)).s7s6s5s4s3s2s1s0)
    #define __ENDIAN_DEFINED
    #define __BSWAP_DEFINED
#endif

/* For everything else, use the compiler's predefined endian macros */
#if !defined(__ENDIAN_DEFINED) && defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) &&   \
    defined(__ORDER_BIG_ENDIAN__)
    #define __ENDIAN_DEFINED
    #define _BYTE_ORDER __BYTE_ORDER__
    #define _LITTLE_ENDIAN __ORDER_LITTLE_ENDIAN__
    #define _BIG_ENDIAN __ORDER_BIG_ENDIAN__
#endif

/* No endian macros found */
#ifndef __ENDIAN_DEFINED
    #error Could not determine CPU byte order
#endif

/* POSIX - http://austingroupbugs.net/view.php?id=162 */
#ifndef BYTE_ORDER
    #define BYTE_ORDER _BYTE_ORDER
#endif
#ifndef LITTLE_ENDIAN
    #define LITTLE_ENDIAN _LITTLE_ENDIAN
#endif
#ifndef BIG_ENDIAN
    #define BIG_ENDIAN _BIG_ENDIAN
#endif

/*
 * Natural to foreign endian helpers defined using bswap
 *
 * MSC can't lift byte swap expressions efficiently so we
 * define host integer swaps using explicit byte swapping.
 */

/* helps to have these function for symmetry */
static inline uint8_t le8(uint8_t x) { return x; }
static inline uint8_t be8(uint8_t x) { return x; }

#if defined(__BSWAP_DEFINED)
    #if _BYTE_ORDER == _BIG_ENDIAN
static inline uint16_t be16(uint16_t x) { return x; }
static inline uint32_t be32(uint32_t x) { return x; }
static inline uint64_t be64(uint64_t x) { return x; }
static inline uint16_t le16(uint16_t x) { return bswap16(x); }
static inline uint32_t le32(uint32_t x) { return bswap32(x); }
static inline uint64_t le64(uint64_t x) { return bswap64(x); }
    #endif
    #if _BYTE_ORDER == _LITTLE_ENDIAN
static inline uint16_t be16(uint16_t x) { return bswap16(x); }
static inline uint32_t be32(uint32_t x) { return bswap32(x); }
static inline uint64_t be64(uint64_t x) { return bswap64(x); }
static inline uint16_t le16(uint16_t x) { return x; }
static inline uint32_t le32(uint32_t x) { return x; }
static inline uint64_t le64(uint64_t x) { return x; }
    #endif

#else
    #define __BSWAP_DEFINED

/*
 * Natural to foreign endian helpers using type punning
 *
 * Recent Clang and GCC lift these expressions to bswap
 * instructions. This makes baremetal code easier.
 */

static inline uint16_t be16(uint16_t x) {
    union {
        uint8_t a[2];
        uint16_t b;
    } y = {.a = {(uint8_t) (x >> 8), (uint8_t) (x)}};
    return y.b;
}

static inline uint16_t le16(uint16_t x) {
    union {
        uint8_t a[2];
        uint16_t b;
    } y = {.a = {(uint8_t) (x), (uint8_t) (x >> 8)}};
    return y.b;
}

static inline uint32_t be32(uint32_t x) {
    union {
        uint8_t a[4];
        uint32_t b;
    } y = {.a = {(uint8_t) (x >> 24), (uint8_t) (x >> 16), (uint8_t) (x >> 8), (uint8_t) (x)}};
    return y.b;
}

static inline uint32_t le32(uint32_t x) {
    union {
        uint8_t a[4];
        uint32_t b;
    } y = {.a = {(uint8_t) (x), (uint8_t) (x >> 8), (uint8_t) (x >> 16), (uint8_t) (x >> 24)}};
    return y.b;
}

static inline uint64_t be64(uint64_t x) {
    union {
        uint8_t a[8];
        uint64_t b;
    } y = {.a = {(uint8_t) (x >> 56), (uint8_t) (x >> 48), (uint8_t) (x >> 40), (uint8_t) (x >> 32),
                 (uint8_t) (x >> 24), (uint8_t) (x >> 16), (uint8_t) (x >> 8), (uint8_t) (x)}};
    return y.b;
}

static inline uint64_t le64(uint64_t x) {
    union {
        uint8_t a[8];
        uint64_t b;
    } y = {.a = {(uint8_t) (x), (uint8_t) (x >> 8), (uint8_t) (x >> 16), (uint8_t) (x >> 24),
                 (uint8_t) (x >> 32), (uint8_t) (x >> 40), (uint8_t) (x >> 48),
                 (uint8_t) (x >> 56)}};
    return y.b;
}

    /*
     * Define byte swaps using the natural endian helpers
     *
     * This method relies on the compiler lifting byte swaps.
     */
    #if _BYTE_ORDER == _BIG_ENDIAN
static inline uint16_t bswap16(uint16_t x) { return le16(x); }
static inline uint32_t bswap32(uint32_t x) { return le32(x); }
static inline uint64_t bswap64(uint64_t x) { return le64(x); }
    #endif

    #if _BYTE_ORDER == _LITTLE_ENDIAN
static inline uint16_t bswap16(uint16_t x) { return be16(x); }
static inline uint32_t bswap32(uint32_t x) { return be32(x); }
static inline uint64_t bswap64(uint64_t x) { return be64(x); }
    #endif
#endif

/*
 * BSD host integer interfaces
 */

#ifndef __HOSTSWAP_DEFINED
static inline uint16_t htobe16(uint16_t x) { return be16(x); }
static inline uint16_t htole16(uint16_t x) { return le16(x); }
static inline uint16_t be16toh(uint16_t x) { return be16(x); }
static inline uint16_t le16toh(uint16_t x) { return le16(x); }

static inline uint32_t htobe32(uint32_t x) { return be32(x); }
static inline uint32_t htole32(uint32_t x) { return le32(x); }
static inline uint32_t be32toh(uint32_t x) { return be32(x); }
static inline uint32_t le32toh(uint32_t x) { return le32(x); }

static inline uint64_t htobe64(uint32_t x) { return be64(x); }
static inline uint64_t htole64(uint32_t x) { return le64(x); }
static inline uint64_t be64toh(uint32_t x) { return be64(x); }
static inline uint64_t le64toh(uint32_t x) { return le64(x); }
#endif

#endif // BSWAP_H