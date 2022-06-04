#define NAPI_VERSION 6
#include <node/node_api.h>

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

// The size of an alloc pool in uint64_t words.
#define POOL_SIZE 1024

// The maximum number of words we'll store on the stack.
#define MAX_STACK_WORDS 32

typedef struct allocation {
    /**
     * The pointer to the allocation in the array buffer.
     */
    uint64_t *data;
    /**
     * The array buffer that the allocation is a part of.
     */
    napi_value array_buffer;
    /**
     * The byte offset of the allocation in the array buffer.
     */
    size_t offset;
} allocation;

/**
 * Reference to the array buffer representing the current allocation pool.
 */
static napi_ref pool_ref;
/**
 * Pointer to the current allocation pool.
 */
static uint64_t *pool_data;
/**
 * Current offset in the allocation pool.
 *
 * Start the offset past the end of the pool. This ensures that the first
 * allocation will always allocate a new pool.
 */
static size_t pool_offset = POOL_SIZE + 1;

static void create_pool(napi_env env) {
    napi_status status;

    napi_value pool;

    status =
        napi_create_arraybuffer(env, POOL_SIZE * sizeof(uint64_t), (void **) &pool_data, &pool);
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

static allocation alloc(napi_env env, size_t word_count) {
    napi_status status;

    allocation a;

    if (word_count < (POOL_SIZE >> 1)) {
        if (word_count + pool_offset > POOL_SIZE) {
            if (pool_ref != NULL) {
                status = napi_reference_unref(env, pool_ref, NULL);
                assert(status == napi_ok);
                status = napi_delete_reference(env, pool_ref);
                assert(status == napi_ok);
            }

            create_pool(env);
        }

        // Get array buffer for the alloc pool reference.
        status = napi_get_reference_value(env, pool_ref, &a.array_buffer);
        if (status != napi_ok) {
            goto fail;
        }

        a.data = &pool_data[pool_offset];
        a.offset = pool_offset * sizeof(uint64_t);

        pool_offset += word_count;
    } else {
        status = napi_create_arraybuffer(env, word_count * sizeof(uint64_t), (void **) &a.data,
                                         &a.array_buffer);
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

static bigint get_bigint_words(napi_env env, napi_callback_info info) {
    napi_status status;

    napi_value argv[1];
    size_t argc = 1;

    bigint b;

    status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    if (status != napi_ok) {
        napi_fatal_error(NULL, 0, "Failed to get callback info", NAPI_AUTO_LENGTH);
    }
    if (argc != 1) {
        napi_throw_type_error(env, "ERR_MISSING_ARGS", "Wrong number of arguments");
        goto fail;
    }

    status = napi_get_value_bigint_words(env, argv[0], NULL, &b.word_count, NULL);
    if (status != napi_ok) {
        napi_throw_type_error(env, "ERR_INVALID_ARG_TYPE", "Expected argument to be a bigint");
        goto fail;
    }

    b.buf = alloc(env, b.word_count);

    int sign_bit;

    status = napi_get_value_bigint_words(env, argv[0], &sign_bit, &b.word_count, b.buf.data);
    if (status != napi_ok) {
        napi_fatal_error(NULL, 0, "Failed to get bigint words", NAPI_AUTO_LENGTH);
    }

    return b;

fail:
    b.buf.data = NULL;
    return b;
}

napi_value to_bytes_le(napi_env env, napi_callback_info info) {
    napi_status status;

    bigint b = get_bigint_words(env, info);
    if (b.buf.data == NULL) {
        return NULL;
    }

    size_t len = 0;
    if (b.word_count > 0) {
        uint64_t tail = b.buf.data[b.word_count - 1];
        len = (b.word_count * sizeof(uint64_t)) - clzb(tail);
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

    bigint b = get_bigint_words(env, info);
    if (b.buf.data == NULL) {
        return NULL;
    }

    size_t len = 0;
    if (b.word_count > 0) {
        uint64_t tail = b.buf.data[b.word_count - 1];
        len = (b.word_count * sizeof(uint64_t)) - clzb(tail);

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

typedef struct uint8_array {
    uint8_t *data;
    size_t len;
} uint8_array;

static uint8_array get_uint8_array(napi_env env, napi_callback_info info) {
    napi_status status;

    napi_value argv[1];
    size_t argc = 1;

    uint8_array a;

    status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    if (status != napi_ok) {
        napi_fatal_error(NULL, 0, "Failed to get callback info", NAPI_AUTO_LENGTH);
    }
    if (argc != 1) {
        napi_throw_type_error(env, "ERR_MISSING_ARGS", "Wrong number of arguments");
        goto fail;
    }

    napi_typedarray_type type;

    status = napi_get_typedarray_info(env, argv[0], &type, &a.len, (void **) &a.data, NULL, NULL);
    if (status != napi_ok || type != napi_uint8_array) {
        napi_throw_type_error(env, "ERR_INVALID_ARG_TYPE", "Expected argument to be a Uint8Array");
        goto fail;
    }

    return a;

fail:
    a.data = NULL;
    return a;
}

// TODO: Use alloc pool for from_bytes as well?

napi_value from_bytes_le(napi_env env, napi_callback_info info) {
    napi_status status;
    napi_value result;

    uint64_t stack_buf[MAX_STACK_WORDS];

    uint8_array a = get_uint8_array(env, info);
    if (a.data == NULL) {
        return NULL;
    }

    size_t word_count = a.len / sizeof(uint64_t);
    size_t remainder = a.len % sizeof(uint64_t);

    // If the data if 8-byte aligned and fits in an exact amount of words, then
    // we can just use the data directly.
    if (((uintptr_t) a.data) % sizeof(uint64_t) == 0 && remainder == 0) {
        status = napi_create_bigint_words(env, 0, word_count, (uint64_t *) a.data, &result);
        if (status != napi_ok) {
            goto fail;
        }
        return result;
    }

    word_count += (remainder > 0);
    remainder = 8 - remainder;

    bool fits_in_stack = word_count <= MAX_STACK_WORDS;

    uint64_t *buf = stack_buf;
    if (!fits_in_stack) {
        buf = malloc(word_count * sizeof(uint64_t));
        if (buf == NULL) {
            napi_throw_error(env, "ERR_MEMORY_ALLOCATION_FAILED", "Failed to allocate memory");
            return NULL;
        }
    }

    memset(&((uint8_t *) buf)[a.len], 0, remainder);
    memcpy(buf, a.data, a.len);

    status = napi_create_bigint_words(env, 0, word_count, (uint64_t *) a.data, &result);
    if (status != napi_ok) {
        goto fail;
    }

    if (!fits_in_stack) {
        free(buf);
    }

    return result;

fail:
    napi_fatal_error(NULL, 0, "Failed to create bigint", NAPI_AUTO_LENGTH);
}

napi_value from_bytes_be(napi_env env, napi_callback_info info) {
    napi_status status;
    napi_value result;

    uint64_t stack_buf[MAX_STACK_WORDS];

    uint8_array a = get_uint8_array(env, info);
    if (a.data == NULL) {
        return NULL;
    }

    size_t word_count = a.len / sizeof(uint64_t);
    size_t remainder = a.len % sizeof(uint64_t);

    word_count += (remainder > 0);
    remainder = 8 - remainder;

    size_t buf_len = word_count * sizeof(uint64_t);
    bool fits_in_stack = word_count <= MAX_STACK_WORDS;

    uint64_t *buf = stack_buf;
    if (!fits_in_stack) {
        buf = malloc(buf_len);
        if (buf == NULL) {
            napi_throw_error(env, "ERR_MEMORY_ALLOCATION_FAILED", "Failed to allocate memory");
            return NULL;
        }
    }

    memset(buf, 0, remainder);
    memcpy(&((uint8_t *) buf)[remainder], a.data, a.len);

    reverse((uint8_t *) buf, buf_len);

    status = napi_create_bigint_words(env, 0, word_count, buf, &result);
    if (status != napi_ok) {
        goto fail;
    }

    if (!fits_in_stack) {
        free(buf);
    }

    return result;

fail:
    napi_fatal_error(NULL, 0, "Failed to create bigint", NAPI_AUTO_LENGTH);
}

NAPI_MODULE_INIT() {

#define EXPORT_FUNCTION(env, fn, name, fail)                                                       \
    do {                                                                                           \
        napi_value fn_value;                                                                       \
        napi_status status;                                                                        \
                                                                                                   \
        status = napi_create_function(env, name, NAPI_AUTO_LENGTH, fn, NULL, &fn_value);           \
        if (status != napi_ok) {                                                                   \
            goto fail;                                                                             \
        }                                                                                          \
                                                                                                   \
        status = napi_set_named_property(env, exports, name, fn_value);                            \
        if (status != napi_ok) {                                                                   \
            goto fail;                                                                             \
        }                                                                                          \
    } while (0)

    EXPORT_FUNCTION(env, to_bytes_le, "toBytesLE", fail);
    EXPORT_FUNCTION(env, to_bytes_be, "toBytesBE", fail);

    EXPORT_FUNCTION(env, from_bytes_le, "fromBytesLE", fail);
    EXPORT_FUNCTION(env, from_bytes_be, "fromBytesBE", fail);

#undef EXPORT_FUNCTION

    return exports;

fail:
    napi_throw_error(env, NULL, "Failed to initialize native module");
    return NULL;
}
