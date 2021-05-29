#pragma once

#include <stddef.h>
#include <stdint.h>

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

struct vf8_buf
{
    char *data;
    size_t data_offset;
    size_t data_size;
};

typedef struct vf8_buf vf8_buf;

vf8_buf* vf8_buf_new(size_t size);
void vf8_buf_destroy(vf8_buf* buf);
void vf8_buf_dump(vf8_buf *buf);

size_t vf8_buf_write_i8(vf8_buf* buf, int8_t num);
size_t vf8_buf_write_i16(vf8_buf* buf, int16_t num);
size_t vf8_buf_write_i32(vf8_buf* buf, int32_t num);
size_t vf8_buf_write_i64(vf8_buf* buf, int64_t num);
size_t vf8_buf_write_bytes(vf8_buf* buf, const char *s, size_t len);

size_t vf8_buf_read_i8(vf8_buf* buf, int8_t *num);
size_t vf8_buf_read_i16(vf8_buf* buf, int16_t *num);
size_t vf8_buf_read_i32(vf8_buf* buf, int32_t *num);
size_t vf8_buf_read_i64(vf8_buf* buf, int64_t *num);
size_t vf8_buf_read_bytes(vf8_buf* buf, char *s, size_t len);

void vf8_buf_reset(vf8_buf* buf);
void vf8_buf_seek(vf8_buf* buf, size_t offset);
char* vf8_buf_data(vf8_buf *buf);
size_t vf8_buf_offset(vf8_buf* buf);

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
