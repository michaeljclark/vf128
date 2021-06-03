#pragma once

#include <stddef.h>
#include <stdint.h>

#include "stdendian.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long s64;
typedef unsigned long long u64;
typedef float f32;
typedef double f64;

/*
 * buffer interface
 */

struct vf8_buf;
struct vf8_span;

typedef struct vf8_buf vf8_buf;
typedef struct vf8_span vf8_span;

struct vf8_span
{
    void *data;
    size_t length;
};

struct vf8_buf
{
    char *data;
    size_t data_offset;
    size_t data_size;
};

vf8_buf* vf8_buf_new(size_t size);
void vf8_buf_destroy(vf8_buf* buf);
void vf8_buf_dump(vf8_buf *buf);

static size_t vf8_buf_write_i8(vf8_buf* buf, int8_t num);
static size_t vf8_buf_write_i16(vf8_buf* buf, int16_t num);
static size_t vf8_buf_write_i32(vf8_buf* buf, int32_t num);
static size_t vf8_buf_write_i64(vf8_buf* buf, int64_t num);
static size_t vf8_buf_write_bytes(vf8_buf* buf, const char *s, size_t len);

static size_t vf8_buf_read_i8(vf8_buf* buf, int8_t *num);
static size_t vf8_buf_read_i16(vf8_buf* buf, int16_t *num);
static size_t vf8_buf_read_i32(vf8_buf* buf, int32_t *num);
static size_t vf8_buf_read_i64(vf8_buf* buf, int64_t *num);
static size_t vf8_buf_read_bytes(vf8_buf* buf, char *s, size_t len);

static void vf8_buf_reset(vf8_buf* buf);
static void vf8_buf_seek(vf8_buf* buf, size_t offset);
static char* vf8_buf_data(vf8_buf *buf);
static size_t vf8_buf_offset(vf8_buf* buf);

/*
 * buffer inline functions
 */

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64)
#ifndef USE_UNALIGNED_ACCESSES
#define USE_UNALIGNED_ACCESSES 1
#endif
#endif

#if defined (_MSC_VER)
#define USE_CRT_MEMCPY 0
#else
#define USE_CRT_MEMCPY 1
#endif

#define CREFL_FN(Y,X) vf8_ ## Y ## _ ## X

#if USE_UNALIGNED_ACCESSES && !USE_CRT_MEMCPY

#define CREFL_BUF_WRITE_IMPL(suffix,T,swap)                                \
static inline size_t CREFL_FN(buf_write,suffix)(vf8_buf *buf, T val)       \
{                                                                          \
    if (buf->data_offset + sizeof(T) > buf->data_size) return 0;           \
    T t = swap(val);                                                       \
    *(T*)(buf->data + buf->data_offset) = t;                               \
    buf->data_offset += sizeof(T);                                         \
    return sizeof(T);                                                      \
}


#define CREFL_BUF_READ_IMPL(suffix,T,swap)                                 \
static inline size_t CREFL_FN(buf_read,suffix)(vf8_buf *buf, T* val)       \
{                                                                          \
    if (buf->data_offset + sizeof(T) > buf->data_size) return 0;           \
    T t;                                                                   \
    t = *(T*)(buf->data + buf->data_offset);                               \
    *val = swap(t);                                                        \
    buf->data_offset += sizeof(T);                                         \
    return sizeof(T);                                                      \
}

#else

#define CREFL_BUF_WRITE_IMPL(suffix,T,swap)                                \
static inline size_t CREFL_FN(buf_write,suffix)(vf8_buf *buf, T val)       \
{                                                                          \
    if (buf->data_offset + sizeof(T) > buf->data_size) return 0;           \
    T t = swap(val);                                                       \
    memcpy(buf->data + buf->data_offset, &t, sizeof(T));                   \
    buf->data_offset += sizeof(T);                                         \
    return sizeof(T);                                                      \
}


#define CREFL_BUF_READ_IMPL(suffix,T,swap)                                 \
static inline size_t CREFL_FN(buf_read,suffix)(vf8_buf *buf, T* val)       \
{                                                                          \
    if (buf->data_offset + sizeof(T) > buf->data_size) return 0;           \
    T t;                                                                   \
    memcpy(&t, buf->data + buf->data_offset, sizeof(T));                   \
    *val = swap(t);                                                        \
    buf->data_offset += sizeof(T);                                         \
    return sizeof(T);                                                      \
}

#endif

CREFL_BUF_WRITE_IMPL(i8,int8_t,le8)
CREFL_BUF_WRITE_IMPL(i16,int16_t,le16)
CREFL_BUF_WRITE_IMPL(i32,int32_t,le32)
CREFL_BUF_WRITE_IMPL(i64,int64_t,le64)

CREFL_BUF_READ_IMPL(i8,int8_t,le8)
CREFL_BUF_READ_IMPL(i16,int16_t,le16)
CREFL_BUF_READ_IMPL(i32,int32_t,le32)
CREFL_BUF_READ_IMPL(i64,int64_t,le64)

static inline size_t vf8_buf_write_bytes(vf8_buf* buf, const char *src, size_t len)
{
    if (buf->data_offset + len > buf->data_size) return 0;
#if USE_CRT_MEMCPY
    memcpy(&buf->data[buf->data_offset], src, len);
#else
    char *dst = &buf->data[buf->data_offset];
    size_t l = len;
    while (l-- > 0) *dst++ = *src++;
#endif
    buf->data_offset += len;
    return len;
}

static inline size_t vf8_buf_read_bytes(vf8_buf* buf, char *dst, size_t len)
{
    if (buf->data_offset + len > buf->data_size) return 0;
#if USE_CRT_MEMCPY
    memcpy(dst, &buf->data[buf->data_offset], len);
#else
    const char *src = &buf->data[buf->data_offset];
    size_t l = len;
    while (l-- > 0) *dst++ = *src++;
#endif
    buf->data_offset += len;
    return len;
}

static inline void vf8_buf_reset(vf8_buf* buf)
{
    buf->data_offset = 0;
}

static inline void vf8_buf_seek(vf8_buf* buf, size_t offset)
{
    buf->data_offset = offset;
}

static inline char* vf8_buf_data(vf8_buf *buf)
{
    return buf->data;
}

static inline size_t vf8_buf_offset(vf8_buf* buf)
{
    return buf->data_offset;
}

static inline vf8_span vf8_buf_remaining(vf8_buf* buf)
{
    vf8_span s = {
        &buf->data[buf->data_offset], buf->data_size - buf->data_offset
    };
    return s;
}

/*
 * floating point helpers
 */

float _f32_inf();
float _f32_nan();
float _f32_snan();
double _f64_inf();
double _f64_nan();
double _f64_snan();

/*
 * integer and floating-point serialization
 */

size_t vf8_asn1_ber_integer_u64_length(const u64 *value);
int vf8_asn1_ber_integer_u64_read(vf8_buf *buf, size_t len, u64 *value);
int vf8_asn1_ber_integer_u64_write(vf8_buf *buf, size_t len, const u64 *value);

size_t vf8_asn1_ber_integer_s64_length(const s64 *value);
int vf8_asn1_ber_integer_s64_read(vf8_buf *buf, size_t len, s64 *value);
int vf8_asn1_ber_integer_s64_write(vf8_buf *buf, size_t len, const s64 *value);

size_t vf8_le_ber_integer_u64_length(const u64 *value);
int vf8_le_ber_integer_u64_read(vf8_buf *buf, size_t len, u64 *value);
int vf8_le_ber_integer_u64_write(vf8_buf *buf, size_t len, const u64 *value);

size_t vf8_le_ber_integer_s64_length(const s64 *value);
int vf8_le_ber_integer_s64_read(vf8_buf *buf, size_t len, s64 *value);
int vf8_le_ber_integer_s64_write(vf8_buf *buf, size_t len, const s64 *value);

size_t vf8_asn1_ber_real_f64_length(const double *value);
int vf8_asn1_ber_real_f64_read(vf8_buf *buf, size_t len, double *value);
int vf8_asn1_ber_real_f64_write(vf8_buf *buf, size_t len, const double *value);

int vf8_f64_read(vf8_buf *buf, double *value);
int vf8_f64_write(vf8_buf *buf, const double *value);

#ifdef __cplusplus
}
#endif
