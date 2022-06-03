#define NAPI_VERSION 6
#include <node/node_api.h>

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define POOL_SIZE 8192

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

typedef struct allocation {
    uint64_t *data;
    napi_value array_buffer;
    size_t offset;
} allocation;

napi_ref pool_ref;

// Start the offset past the end of the pool. This ensures that the first
// allocation will always allocate a new pool.
size_t pool_offset = POOL_SIZE + 1;
uint8_t *pool_data;

void create_pool(napi_env env) {
    napi_status status;

    napi_value pool;

    status = napi_create_arraybuffer(env, POOL_SIZE, (void **) &pool_data, &pool);
    if (status != napi_ok) {
        goto fail;
    }

    status = napi_create_reference(env, pool, 1, &pool_ref);
    if (status != napi_ok) {
        goto fail;
    }

    pool_offset = 0;
    return;

fail:
    napi_fatal_error(NULL, 0, "Failed to create alloc pool", NAPI_AUTO_LENGTH);
}

allocation alloc(napi_env env, size_t word_count) {
    napi_status status;

    allocation a;

    size_t size = word_count * sizeof(uint64_t);
    if (size < (POOL_SIZE >> 1)) {
        if (size + pool_offset > POOL_SIZE) {
            if (pool_ref != NULL) {
                status = napi_reference_unref(env, pool_ref, NULL);
                assert(status == napi_ok);
            }

            create_pool(env);
        }

        // Get array buffer for the alloc pool reference.
        status = napi_get_reference_value(env, pool_ref, &a.array_buffer);
        if (status != napi_ok) {
            goto fail;
        }

        // &pool_data[pool_offset] is always 8-byte aligned.
        a.data = (uint64_t *) &pool_data[pool_offset];
        a.offset = pool_offset;

        pool_offset += size;
    } else {
        status = napi_create_arraybuffer(env, size, (void **) &a.data, &a.array_buffer);
        if (status != napi_ok) {
            napi_fatal_error(NULL, 0, "Failed to create array buffer", NAPI_AUTO_LENGTH);
        }

        a.offset = 0;
    }

    return a;

fail:
    napi_fatal_error(NULL, 0, "Failed to get reference for alloc pool", NAPI_AUTO_LENGTH);
}

typedef struct bigint {
    size_t word_count;
    allocation buf;
} bigint;

bool get_bigint_words(napi_env env, napi_callback_info info, bigint *b) {
    napi_status status;

    napi_value argv[1];
    size_t argc = 1;

    status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    if (status != napi_ok) {
        napi_fatal_error(NULL, 0, "Failed to get callback info", NAPI_AUTO_LENGTH);
    }
    if (argc != 1) {
        napi_throw_type_error(env, "ERR_MISSING_ARGS", "Wrong number of arguments");
        return false;
    }

    status = napi_get_value_bigint_words(env, argv[0], NULL, &b->word_count, NULL);
    if (status != napi_ok) {
        napi_throw_type_error(env, "ERR_INVALID_ARG_TYPE", "Expected argument to be typeof bigint");
        return false;
    }

    b->buf = alloc(env, b->word_count);

    int sign_bit;

    status = napi_get_value_bigint_words(env, argv[0], &sign_bit, &b->word_count, b->buf.data);
    if (status != napi_ok) {
        napi_fatal_error(NULL, 0, "Failed to get bigint words", NAPI_AUTO_LENGTH);
    }

    return true;
}

napi_value to_bytes_le(napi_env env, napi_callback_info info) {
    napi_status status;

    bigint b;

    if (!get_bigint_words(env, info, &b)) {
        return NULL;
    }

    size_t len = b.word_count * sizeof(uint64_t);
    if (b.word_count > 0) {
        uint64_t tail = b.buf.data[b.word_count - 1];
        len -= clzb(tail);
    }

    napi_value result;

    status = napi_create_typedarray(env, napi_uint8_array, len, b.buf.array_buffer, b.buf.offset,
                                    &result);
    if (status != napi_ok) {
        napi_fatal_error(NULL, 0, "Failed to create Uint8Array", NAPI_AUTO_LENGTH);
    }

    return result;
}

napi_value to_bytes_be(napi_env env, napi_callback_info info) {
    napi_status status;

    bigint b;

    if (!get_bigint_words(env, info, &b)) {
        return NULL;
    }

    size_t len = b.word_count * sizeof(uint64_t);
    if (b.word_count > 0) {
        uint64_t tail = b.buf.data[b.word_count - 1];
        len -= clzb(tail);

        reverse((uint8_t *) b.buf.data, len);
    }

    napi_value result;

    status = napi_create_typedarray(env, napi_uint8_array, len, b.buf.array_buffer, b.buf.offset,
                                    &result);
    if (status != napi_ok) {
        napi_fatal_error(NULL, 0, "Failed to create Uint8Array", NAPI_AUTO_LENGTH);
    }

    return result;
}

NAPI_MODULE_INIT() {
    napi_status status;

    napi_value to_bytes_le_fn;
    napi_value to_bytes_be_fn;

    status = napi_create_function(env, "toBytesLE", NAPI_AUTO_LENGTH, to_bytes_le, NULL,
                                  &to_bytes_le_fn);
    if (status != napi_ok) {
        goto fail;
    }

    status = napi_create_function(env, "toBytesBE", NAPI_AUTO_LENGTH, to_bytes_be, NULL,
                                  &to_bytes_be_fn);
    if (status != napi_ok) {
        goto fail;
    }

    status = napi_set_named_property(env, exports, "toBytesLE", to_bytes_le_fn);
    if (status != napi_ok) {
        goto fail;
    }

    status = napi_set_named_property(env, exports, "toBytesBE", to_bytes_be_fn);
    if (status != napi_ok) {
        goto fail;
    }

    return exports;

fail:
    napi_throw_error(env, NULL, "Failed to initialize native module");
    return NULL;
}
