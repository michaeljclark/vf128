#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cinttypes>

#include <string>
#include <limits>

#include "vf128.h"
#include "stdbits.h"
#include "stdendian.h"

#define DEBUG_ENCODING 0

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64)
#ifndef USE_UNALIGNED_ACCESSES
#define USE_UNALIGNED_ACCESSES 1
#endif
#endif

/*
 * buffer implementation
 */

vf8_buf* vf8_buf_new(size_t size)
{
    vf8_buf *buf = (vf8_buf*)malloc(sizeof(vf8_buf));

    buf->data_offset = 0;
    buf->data_size = size;
    buf->data = (char*)malloc(buf->data_size);
    memset(buf->data, 0, buf->data_size);

    return buf;
}

void vf8_buf_destroy(vf8_buf* buf)
{
    free(buf->data);
    free(buf);
}

void vf8_buf_reset(vf8_buf* buf)
{
    buf->data_offset = 0;
}

void vf8_buf_seek(vf8_buf* buf, size_t offset)
{
    buf->data_offset = offset;
}

char* vf8_buf_data(vf8_buf *buf)
{
    return buf->data;
}

size_t vf8_buf_offset(vf8_buf* buf)
{
    return buf->data_offset;
}

static std::string _to_binary_block(uint64_t symbol, size_t bit_width)
{
    static const char* arr[] = { "▄", "▟", "▙", "█" };
    std::string s;
    for (intptr_t i = bit_width-2; i >= 0; i-=2) {
        s.append(arr[(symbol>>i) & 3]);
    }
    return s;
}

static std::string _to_binary_text(uint64_t symbol, intptr_t b, intptr_t e)
{
    std::string s;
    for (intptr_t i = b; i >= e; i--) {
        s.append(1, '0' + ((symbol>>i) & 1));
    }
    return s;
}

void vf8_buf_dump(vf8_buf *buf)
{
    intptr_t stride = 16;
    for (intptr_t i = 0; i < buf->data_offset; i += stride) {
        printf("      ");
        for (intptr_t j = i+stride-1; j >= i; j--) {
            if (j >= buf->data_offset) printf("     ");
            else printf(" 0x%02hhX", buf->data[j]);
        }
        printf("\n");
        printf("%04zX: ", i & 0xffff);
        for (intptr_t j = i+stride-1; j >= i; j--) {
            if (j >= buf->data_offset) printf(" ░░░░");
            else printf(" %s", _to_binary_block(buf->data[j], 8).c_str());
        }
        printf("\n");
    }
}

template <typename INT, typename SWAP>
static size_t vf8_buf_write_int(vf8_buf *buf, INT num, SWAP f);
template <typename INT, typename SWAP>
static size_t vf8_buf_read_int(vf8_buf *buf, INT *num, SWAP f);

size_t vf8_buf_write_i8(vf8_buf* buf, int8_t num)
{ return vf8_buf_write_int<int8_t>(buf,num,le8); }
size_t vf8_buf_write_i16(vf8_buf* buf, int16_t num)
{ return vf8_buf_write_int<int16_t>(buf,num,le16); }
size_t vf8_buf_write_i32(vf8_buf* buf, int32_t num)
{ return vf8_buf_write_int<int32_t>(buf,num,le32); }
size_t vf8_buf_write_i64(vf8_buf* buf, int64_t num)
{ return vf8_buf_write_int<int64_t>(buf,num,le64); }

size_t vf8_buf_read_i8(vf8_buf* buf, int8_t *num)
{ return vf8_buf_read_int<int8_t>(buf,num,le8); }
size_t vf8_buf_read_i16(vf8_buf* buf, int16_t *num)
{ return vf8_buf_read_int<int16_t>(buf,num,le16); }
size_t vf8_buf_read_i32(vf8_buf* buf, int32_t *num)
{ return vf8_buf_read_int<int32_t>(buf,num,le32); }
size_t vf8_buf_read_i64(vf8_buf* buf, int64_t *num)
{ return vf8_buf_read_int<int64_t>(buf,num,le64); }

template <typename INT, typename SWAP>
static size_t vf8_buf_write_int(vf8_buf *buf, INT num, SWAP f)
{
    if (buf->data_offset + sizeof(num) > buf->data_size) return 0;
    INT t = f(num);
    memcpy(buf->data + buf->data_offset, &t, sizeof(INT));
    buf->data_offset += sizeof(num);
    return sizeof(num);
}

template <typename INT, typename SWAP>
static size_t vf8_buf_read_int(vf8_buf *buf, INT *num, SWAP f)
{
    if (buf->data_offset + sizeof(*num) > buf->data_size) return 0;
    INT t;
    memcpy(&t, buf->data + buf->data_offset, sizeof(INT));
    *num = f(t);
    buf->data_offset += sizeof(*num);
    return sizeof(*num);
}

size_t vf8_buf_write_bytes(vf8_buf* buf, const char *s, size_t len)
{
    if (buf->data_offset + len > buf->data_size) return 0;
    memcpy(&buf->data[buf->data_offset], s, len);
    buf->data_offset += len;
    return len;
}

size_t vf8_buf_read_bytes(vf8_buf* buf, char *s, size_t len)
{
    if (buf->data_offset + len > buf->data_size) return 0;
    memcpy(s, &buf->data[buf->data_offset], len);
    buf->data_offset += len;
    return len;
}

/*
 * ISO/IEC 8825-1:2003 8.3 integer
 *
 * read and write integer
 */

size_t vf8_asn1_ber_integer_u64_length(const u64 *value)
{
    return *value == 0 ? 1 : 8 - (clz(*value) / 8);
}

int vf8_asn1_ber_integer_u64_read(vf8_buf *buf, size_t len, u64 *value)
{
    u64 v = 0, o = 0;
    size_t shift = (64 - len * 8);

    if (len > 8) {
        goto err;
    }
    if (vf8_buf_read_bytes(buf, (char*)&o, len) != len) {
        goto err;
    }

#if _BYTE_ORDER == _BIG_ENDIAN
    v = be64(o >> shift);
#elif _BYTE_ORDER == _LITTLE_ENDIAN
    v = be64(o << shift);
#endif

    *value = v;
    return 0;
err:
    *value = 0;
    return -1;
}

int vf8_asn1_ber_integer_u64_write(vf8_buf *buf, size_t len, const u64 *value)
{
    u64 v = 0, o = 0;
    size_t shift = (64 - len * 8);

    if (len < 1 || len > 8) {
        goto err;
    }

#if _BYTE_ORDER == _BIG_ENDIAN
    o = be64(*value) << shift;
#elif _BYTE_ORDER == _LITTLE_ENDIAN
    o = be64(*value) >> shift;
#endif

    if (vf8_buf_write_bytes(buf, (const char*)&o, len) != len) {
        goto err;
    }

    return 0;
err:
    return -1;
}

/*
 * ISO/IEC 8825-1:2003 8.3 integer
 *
 * read and write integer
 */

size_t vf8_le_ber_integer_u64_length(const u64 *value)
{
    return *value == 0 ? 1 : 8 - (clz(*value) / 8);
}

int vf8_le_ber_integer_u64_read(vf8_buf *buf, size_t len, u64 *value)
{
    u64 v = 0, o = 0;

    if (len > 8) {
        goto err;
    }

#if USE_UNALIGNED_ACCESSES
    if (vf8_buf_read_bytes(buf, (char*)value, len) != len) {
        goto err;
    }
#else
    if (vf8_buf_read_bytes(buf, (char*)&o, len) != len) {
        goto err;
    }

    *value = le64(o);
#endif

    return 0;
err:
    *value = 0;
    return -1;
}

int vf8_le_ber_integer_u64_write(vf8_buf *buf, size_t len, const u64 *value)
{
    u64 v = 0, o = 0;

    if (len < 1 || len > 8) {
        goto err;
    }

#if USE_UNALIGNED_ACCESSES
    if (vf8_buf_write_bytes(buf, (const char*)value, len) != len) {
        goto err;
    }
#else
    o = le64(*value);

    if (vf8_buf_write_bytes(buf, (const char*)&o, len) != len) {
        goto err;
    }
#endif

    return 0;
err:
    return -1;
}

/*
 * ASN.1 does not distinguish between signed and unsigned integers.
 * signed number deserialization requires that values are sign-extended.
 * to calculate the length negative values are complemented and 1-bit is
 * reserved for the sign. the values are stored in two's complement.
 *
 * - 0x000000000000007f -> 0x7f
 * - 0x0000000000000080 -> 0x0080
 * - 0xffffffffffffff80 -> 0x80
 * - 0xffffffffffffff7f -> 0xff7f
 */
size_t vf8_asn1_ber_integer_s64_length(const s64 *value)
{
    s64 v = *value;
    return v == 0 ? 1 : 8 - ((clz(v < 0 ? ~v : v)-1) / 8);
}

static s64 _sign_extend_s64(s64 x, size_t y) { return ((s64)(x << y)) >> y; }

int vf8_asn1_ber_integer_s64_read(vf8_buf *buf, size_t len, s64 *value)
{
    int ret = vf8_asn1_ber_integer_u64_read(buf, len, (u64*)value);
    if (ret == 0) {
        *value = _sign_extend_s64(*value, 64-(len << 3));
    }
    return ret;
}

int vf8_asn1_ber_integer_s64_write(vf8_buf *buf, size_t len, const s64 *value)
{
    return vf8_asn1_ber_integer_u64_write(buf, len, (const u64*)value);
}

size_t vf8_le_ber_integer_s64_length(const s64 *value)
{
    return vf8_asn1_ber_integer_s64_length(value);
}

int vf8_le_ber_integer_s64_read(vf8_buf *buf, size_t len, s64 *value)
{
    int ret = vf8_le_ber_integer_u64_read(buf, len, (u64*)value);
    if (ret == 0) {
        *value = _sign_extend_s64(*value, 64-(len << 3));
    }
    return ret;
}

int vf8_le_ber_integer_s64_write(vf8_buf *buf, size_t len, const s64 *value)
{
    return vf8_le_ber_integer_u64_write(buf, len, (const u64*)value);
}

/*
 * IEEE 754 encoding and decoding functions
 */

union f32_bits { f32 f; u32 u; };
union f64_bits { f64 f; u64 u; };

struct f32_struct { u32 mant : 23; u32 exp : 8;  u32 sign : 1; };
struct f64_struct { u64 mant : 52; u64 exp : 11; u64 sign : 1; };

enum : u32 {
    f32_exp_size = 8,
    f32_mant_size = 23,

    f32_mant_shift = 0,
    f32_exp_shift = f32_mant_size,
    f32_sign_shift = f32_mant_size + f32_exp_size,

    f32_mant_mask = (1 << f32_mant_size) - 1,
    f32_exp_mask = (1 << f32_exp_size) - 1,
    f32_sign_mask = 1,

    f32_mant_prefix = (1ull << f32_mant_size),
    f32_exp_bias = (1 << (f32_exp_size-1)) - 1
};

enum : u64 {
    f64_exp_size = 11,
    f64_mant_size = 52,

    f64_mant_shift = 0,
    f64_exp_shift = f64_mant_size,
    f64_sign_shift = f64_mant_size + f64_exp_size,

    f64_mant_mask = (1ull << f64_mant_size) - 1,
    f64_exp_mask = (1ull << f64_exp_size) - 1,
    f64_sign_mask = 1ull,

    f64_mant_prefix = (1ull << f64_mant_size),
    f64_exp_bias = (1 << (f64_exp_size-1)) - 1
};

static f32 f32_from_bits(u32 v) { union { u32 u; f32 f; } x = { v }; return x.f; }
static u32 f32_to_bits(f32 x)   { union { f32 f; u32 u; } v = { x }; return v.u; }

static u32 f32_mant_dec(f32 x)  { return ( f32_to_bits(x) >> f32_mant_shift ) & f32_mant_mask; }
static u32 f32_exp_dec(f32 x)   { return ( f32_to_bits(x) >> f32_exp_shift  ) & f32_exp_mask;  }
static u32 f32_sign_dec(f32 x)  { return ( f32_to_bits(x) >> f32_sign_shift ) & f32_sign_mask; }

static u32 f32_mant_enc(u32 v)  { return ((v & f32_mant_mask) << f32_mant_shift); }
static u32 f32_exp_enc(u32 v)   { return ((v & f32_exp_mask)  << f32_exp_shift);  }
static u32 f32_sign_enc(u32 v)  { return ((v & f32_sign_mask) << f32_sign_shift); }

static int f32_is_zero(f32 x)   { return f32_exp_dec(x) == 0            && f32_mant_dec(x) == 0; }
static int f32_is_inf(f32 x)    { return f32_exp_dec(x) == f32_exp_mask && f32_mant_dec(x) == 0; }
static int f32_is_nan(f32 x)    { return f32_exp_dec(x) == f32_exp_mask && f32_mant_dec(x) != 0; }
static int f32_is_denorm(f32 x) { return f32_exp_dec(x) == 0            && f32_mant_dec(x) != 0; }

static f32_struct f32_unpack_float(f32 x)
{
    return { f32_mant_dec(x), f32_exp_dec(x), f32_sign_dec(x) };
}

static f32 f32_pack_float(f32_struct s)
{
    return f32_from_bits(f32_mant_enc(s.mant) | f32_exp_enc(s.exp) | f32_sign_enc(s.sign));
}

static f64 f64_from_bits(u64 v) { union { u64 u; f64 f; } x = { v }; return x.f; }
static u64 f64_to_bits(f64 x)   { union { f64 f; u64 u; } v = { x }; return v.u; }

static u64 f64_mant_dec(f64 x)  { return ( f64_to_bits(x) >> f64_mant_shift ) & f64_mant_mask; }
static u64 f64_exp_dec(f64 x)   { return ( f64_to_bits(x) >> f64_exp_shift  ) & f64_exp_mask;  }
static u64 f64_sign_dec(f64 x)  { return ( f64_to_bits(x) >> f64_sign_shift ) & f64_sign_mask; }

static u64 f64_mant_enc(u64 v)  { return ((v & f64_mant_mask) << f64_mant_shift); }
static u64 f64_exp_enc(u64 v)   { return ((v & f64_exp_mask)  << f64_exp_shift);  }
static u64 f64_sign_enc(u64 v)  { return ((v & f64_sign_mask) << f64_sign_shift); }

static int f64_is_zero(f64 x)   { return f64_exp_dec(x) == 0            && f64_mant_dec(x) == 0; }
static int f64_is_inf(f64 x)    { return f64_exp_dec(x) == f64_exp_mask && f64_mant_dec(x) == 0; }
static int f64_is_nan(f64 x)    { return f64_exp_dec(x) == f64_exp_mask && f64_mant_dec(x) != 0; }
static int f64_is_denorm(f64 x) { return f64_exp_dec(x) == 0            && f64_mant_dec(x) != 0; }

static f64_struct f64_unpack_float(f64 x)
{
    return { f64_mant_dec(x), f64_exp_dec(x), f64_sign_dec(x) };
}

static f64 f64_pack_float(f64_struct s)
{
    return f64_from_bits( f64_mant_enc(s.mant) | f64_exp_enc(s.exp) | f64_sign_enc(s.sign) );
}

float _f32_inf() { return std::numeric_limits<float>::infinity(); }
float _f32_nan() { return std::numeric_limits<float>::quiet_NaN(); }
float _f32_snan() { return std::numeric_limits<float>::signaling_NaN(); }
double _f64_inf() { return std::numeric_limits<double>::infinity(); }
double _f64_nan() { return std::numeric_limits<double>::quiet_NaN(); }
double _f64_snan() { return std::numeric_limits<double>::signaling_NaN(); }

/*
 * ISO/IEC 8825-1:2003 8.5 real
 *
 * read and write real
 *
 * ASN.1 REAL encoding bits from first content byte
 *
 * - 8.5.6 encoding-format
 *   - [8]   0b'1 = binary
 *   - [8:7] 0b'00 = decimal
 *   - [8:7] 0b'01 = special real value
 * - 8.5.7.1 sign (if bit 8 = 0b1)
 *   - [7]   sign S
 * - 8.5.7.2 base (if bit 8 = 0b1)
 *   - [6:5] 0b'00 = base 2
 *   - [6:5] 0b'01 = base 8
 *   - [6:5] 0b'10 = base 16
 *   - [6:5] 0b'11 = reserved
 * - 8.5.7.3 scale-factor (if bit 8 = 0b1)
 *   - [4:3] unsigned scale factor F
 * - 8.5.7.4 exponent-format (if bit 8 = 0b1)
 *   - [2:1] 0b'00 = 1-byte signed exponent in 2nd octet
 *   - [2:1] 0b'01 = 2-byte signed exponent in 2nd to 3rd octet
 *   - [2:1] 0b'10 = 3-byte signed exponent in 2nd to 4rd octet
 *   - [2:1] 0b'11 = 2nd octet contains number of exponent octets
 * - 8.5.8 decimal encoding (if bit 8:7 = 0b00)
 *   - [8:1] 0b'00000001 ISO 6093 NR1 form
 *   - [8:1] 0b'00000010 ISO 6093 NR2 form
 *   - [8:1] 0b'00000011 ISO 6093 NR3 form
 * - 8.5.9 special real value (if bit 8:7 = 0b01)
 *   - [8:1] 0b'01000000 Value is PLUS-INFINITY
 *   - [8:1] 0b'01000001 Value is MINUS-INFINITY
 *   - [8:1] 0b'01000010 Value is NOT-A-NUMBER
 *   - [8:1] 0b'01000011 Value is minus zero
 *
 *  binary encoding: M = S × N × 2^F x (2,8,16)^E
 *  decimal encoding: /[0-9]+\.[0-9]+([eE][+-]?[0-9]+)?/
 */

enum {
    _real_fmt_shift         = 6,
    _real_fmt_mask          = 0b11,
    _real_base_shift        = 4,
    _real_base_mask         = 0b11,
    _real_scale_shift       = 2,
    _real_scale_mask        = 0b11,
    _real_exp_shift         = 0,
    _real_exp_mask          = 0b11,
};

enum _real_fmt {
    _real_fmt_decimal       = 0b00,
    _real_fmt_special       = 0b01,
    _real_fmt_binary_pos    = 0b10,
    _real_fmt_binary_neg    = 0b11
};

enum _real_base {
    _real_base_2            = 0b00,
    _real_base_8            = 0b01,
    _real_base_16           = 0b10
};

enum _real_exp {
    _real_exp_1             = 0b00,
    _real_exp_2             = 0b01,
    _real_exp_3             = 0b10,
    _real_exp_n             = 0b11
};

enum _real_decimal_nr {
    _real_decimal_nr_1      = 0b00000001,
    _real_decimal_nr_2      = 0b00000010,
    _real_decimal_nr_3      = 0b00000011,
};

enum _real_special {
    _real_special_pos_inf   = 0b01000000,
    _real_special_neg_inf   = 0b01000001,
    _real_special_neg_zero  = 0b01000010,
    _real_special_nan       = 0b01000011,
};

static _real_fmt _asn1_real_format(u8 x)
{
    return (_real_fmt)((x >> _real_fmt_shift) & _real_fmt_mask);
}

static _real_exp _asn1_real_exp(u8 x)
{
    return (_real_exp)((x >> _real_exp_shift) & _real_exp_mask);
}

static u8 _asn1_real_binary_full(bool sign, _real_exp exp, _real_base base, u8 scale)
{
    return 0x80 | ((u8)sign<<6)  | (((u8)base)<<4) | ((scale&3)<<2) | (u8)exp;
}

static u8 _asn1_real_binary(bool sign, _real_exp exponent)
{
    return _asn1_real_binary_full(sign, exponent, _real_base_2, 0);
}

/*
 * f64_real_data contains fraction, signed exponent, their
 * encoded lengths and flags for sign, infinity, nan and zero.
 */
struct f64_real_data
{
    u64 frac;
    s64 sexp;
    size_t frac_len, exp_len;
    bool sign : 1, inf : 1, nan : 1, zero : 1;
};

/*
 * IEEE 754 exponent is relative to the msb of the mantissa
 * ASN.1 exponent is relative to the lsb of the mantissa
 *
 * right-justify the fraction with the least significant set bit in bit 0,
 * first adding the IEEE 754 implied leading digit 0b1.xxx
 */
static f64_real_data f64_asn1_data_get(double value)
{
    u64 frac;
    s64 sexp;
    size_t frac_tz, frac_lz;

    sexp = (s64)f64_exp_dec(value);
    frac = (s64)f64_mant_dec(value) | (-(s64)(sexp > 0) & f64_mant_prefix);
    frac_tz = ctz(frac);
    frac_lz = clz(frac);
    frac >>= frac_tz;

    if (sexp > 0) {
        sexp += frac_lz + frac_tz - 63 - f64_exp_bias;
    }

    return f64_real_data {
        frac, sexp,
        vf8_asn1_ber_integer_u64_length(&frac),
        vf8_asn1_ber_integer_s64_length(&sexp),
        !!f64_sign_dec(value), !!f64_is_inf(value),
        !!f64_is_nan(value), !!f64_is_zero(value)
    };
}

size_t vf8_asn1_ber_real_f64_length(const double *value)
{
    f64_real_data d = f64_asn1_data_get(*value);

    if (d.zero) {
        return d.sign ? 1 : 3;
    } else if (d.inf || d.nan) {
        return 1;
    } else {
        return 1 + d.exp_len + d.frac_len;
    }
}

int vf8_asn1_ber_real_f64_read(vf8_buf *buf, size_t len, double *value)
{
    int8_t b;
    double v = 0;
    _real_fmt fmt;
    _real_exp exp_mode;
    size_t frac_len;
    size_t exp_len;
    u64 frac;
    s64 sexp;
    u64 fexp;
    bool sign;
    size_t frac_lz;

    if (vf8_buf_read_i8(buf, &b) != 1) {
        goto err;
    }
    fmt = _asn1_real_format(b);
    switch (b) {
    case _real_special_pos_inf:  *value = std::numeric_limits<f64>::infinity();  return 0;
    case _real_special_neg_inf:  *value = -std::numeric_limits<f64>::infinity(); return 0;
    case _real_special_neg_zero: *value = -0.0;     return 0;
    case _real_special_nan:      *value = std::numeric_limits<f64>::quiet_NaN();  return 0;
    default: break;
    }
    switch (fmt) {
    case _real_fmt_binary_pos: sign = false; break;
    case _real_fmt_binary_neg: sign = true; break;
    default: return -1;
    }
    exp_mode = _asn1_real_exp(b);
    switch(exp_mode) {
    case _real_exp_1: exp_len = 1; break;
    case _real_exp_2: exp_len = 2; break;
    default: return -1;
    }
    frac_len = len - exp_len - 1;

    if (vf8_asn1_ber_integer_s64_read(buf, exp_len, &sexp) < 0) {
        goto err;
    }
    if (vf8_asn1_ber_integer_u64_read(buf, frac_len, &frac) < 0) {
        goto err;
    }
    frac_lz = clz(frac);

    /*
     * IEEE 754 exponent is relative to the msb of the mantissa
     * ASN.1 exponent is relative to the lsb of the mantissa
     *
     * left-justify the fraction with the most significant set bit in bit 51
     * (0-indexed) cropping off the IEEE 754 implied leading digit 0b1.xxx
     */
    if (frac == 1 && sexp == 0) {
        frac = 0;
        fexp = f64_exp_bias;
    } else if (frac == 0 && sexp == 0) {
        fexp = 0;
    } else {
        frac = (frac << (frac_lz + 1)) >> (64 - f64_mant_size);
        fexp = f64_exp_bias + 63 + sexp - frac_lz;
    }
    if (fexp > f64_exp_mask || frac > f64_mant_mask) {
        return -1;
    }
    v = f64_pack_float(f64_struct{frac, fexp, sign});

    *value = v;
    return 0;
err:
    *value = 0;
    return -1;
}

int vf8_asn1_ber_real_f64_write(vf8_buf *buf, size_t len, const double *value)
{
    f64_real_data d = f64_asn1_data_get(*value);

    int8_t b;

    if (d.zero && d.sign) {
        b = _real_special_neg_zero;
    }
    else if (d.inf) {
        b = d.sign ? _real_special_neg_inf : _real_special_pos_inf;
    }
    else if (d.nan) {
        b = _real_special_nan;
    } else {
        _real_exp exp_code;
        switch(d.exp_len) {
        case 1: exp_code = _real_exp_1; break;
        case 2: exp_code = _real_exp_2; break;
        default: return -1;
        }
        b = _asn1_real_binary(d.sign, exp_code);
    }
    if (vf8_buf_write_i8(buf, b) != 1) {
        goto err;
    }
    if ((d.zero && d.sign) || d.inf || d.nan) {
        return 0;
    }
    if (vf8_asn1_ber_integer_s64_write(buf, d.exp_len, &d.sexp) < 0) {
        goto err;
    }
    if (vf8_asn1_ber_integer_u64_write(buf, d.frac_len, &d.frac) < 0) {
        goto err;
    }

    return 0;
err:
    return -1;
}

/*
 * vf8 compressed float
 */

/*
 * vf8_f64_data contains fraction, signed exponent, their
 * encoded lengths and flags for sign, infinity, nan and zero.
 */
struct vf8_f64_data
{
    bool sign;
    s64 sexp;
    u64 frac;
};

/*
 * extract exponent and left-justified fraction
 */
static vf8_f64_data vf8_f64_data_get(double value)
{
    bool sign = !!f64_sign_dec(value);
    s64 sexp = (s64)f64_exp_dec(value) - f64_exp_bias;
    u64 frac = (s64)f64_mant_dec(value) << (f64_exp_size + 1);

    return vf8_f64_data { sign, sexp, frac };
}

#if DEBUG_ENCODING
static void _vf8_debug_pack(double v, u8 pre, s64 vp_exp, u64 vp_man, s64 vd_exp, u64 vd_man)
{
    bool vf_inl = (pre >> 7) & 1;
    bool vf_sgn = (pre >> 6) & 1;
    int vf_exp = (pre >> 4) & 3;
    int vf_man = pre & 15;

    printf("\n%16s %20s -> %18s %5s -> %1s %1s %2s %4s %4s\n",
        "value (dec)", "value (hex)", "fraction", "exp",
        "i", "s", "ex", "mant", "len");
    printf("%16f %20a    0x%016llx %05lld    %1u %1u %2s %4s",
        v, v, vp_man, vp_exp, vf_inl, vf_sgn,
        _to_binary_text(vf_exp, 1, 0).c_str(),
        _to_binary_text(vf_man, 3, 0).c_str());

    printf(" [%02d] { pre=0x%02hhx", 1 + (vf_inl ? 0 : vf_exp + vf_man), pre);
    if (!vf_inl && vf_man) {
        printf(" man=0x%02llx", vd_man);
    }
    if (!vf_inl && vf_exp) {
        printf(" exp=%lld", vd_exp);
    }
    printf(" }\n");
}
#endif

int vf8_f64_read(vf8_buf *buf, double *value)
{
    s8 pre;
	double v = 0;
    bool vf_inl;
    bool vf_sgn;
    int vf_exp;
    int vf_man;
    u64 vr_man = 0;
    s64 vr_exp = 0;
    u64 vp_man = 0;
    s64 vp_exp = 0;

    if (vf8_buf_read_i8(buf, &pre) != 1) {
        goto err;
    }

    vf_inl = (pre >> 7) & 1;
    vf_sgn = (pre >> 6) & 1;
    vf_exp = (pre >> 4) & 3;
    vf_man = pre & 15;

    if (!vf_inl) {
        if (vf_exp && vf8_le_ber_integer_s64_read(buf, vf_exp, &vr_exp) < 0) {
            goto err;
        }
        if (vf_man && vf8_le_ber_integer_u64_read(buf, vf_man, &vr_man) < 0) {
            goto err;
        }
    }

    if (vf_inl) {
        if (vf_exp == 0) {
            if (vf_man > 0) {
                size_t lz = clz((u64)vf_man);
                vp_exp = f64_exp_bias + 59 - lz;
                vp_man = ((u64)vf_man << (lz + 1)) >> (f64_exp_size + 1);
            } else {
                vp_exp = 0;
                vp_man = 0;
            }
        }
        else {
            vp_exp = vf_exp == 3 ? f64_exp_mask : f64_exp_bias + vf_exp - 1;
            vp_man = (u64)vf_man << (f64_mant_size - 4);
        }
    }
    else {
        size_t lz = clz(vr_man);
        if (vf_exp == 0) {
            /* if exponent is zero, the mantissa contains an integer
             * add its width to the exponent and truncate the leading 1 */
            vp_exp = f64_exp_bias + 63 - lz;
            vp_man = (u64)vr_man << (lz + 1) >> (f64_exp_size + 1);
        }
        else {
            /* left-justify the mantissa and truncate the leading 1 */
            vp_exp = f64_exp_bias + vr_exp;
            vp_man = (u64)vr_man << (lz + 1) >> (f64_exp_size + 1);
        }
    }

    v = f64_pack_float(f64_struct{vp_man, (u64)vp_exp, vf_sgn});
    *value = v;

#if DEBUG_ENCODING
    _vf8_debug_pack(v, pre, vp_exp - f64_exp_bias, vp_man << 12, vr_exp, vr_man);
#endif

    return 0;
err:
    *value = 0;
    return -1;
}

enum : u64 {
    u64_msb = 0x8000000000000000ull,
    u64_msn = 0xf000000000000000ull
};

int vf8_f64_write(vf8_buf *buf, const double *value)
{
    s8 pre;
    double v = *value;
	vf8_f64_data d = vf8_f64_data_get(v);
    bool vf_inl = 0;
    bool vf_sgn = d.sign;
    int vf_exp;
    int vf_man;
    u64 vw_man = 0;
    s64 vw_exp = 0;

    // Inf/NaN
    if (d.sexp == f64_exp_bias + 1) {
        vf_inl = 1;
        vf_exp = 3;
        vf_man = d.frac >> 60;
    }
    // Subnormal and Zero
    else if (d.sexp == -f64_exp_bias) {
        if (d.frac == 0) {
            vf_inl = 1;
            vf_exp = 0;
            vf_man = 0;
        } else {
            /* remove leading 1 and renormalize non zero fraction */
            size_t frac_lz = clz(d.frac);
            d.sexp -= frac_lz;
            d.frac <<= frac_lz + 1;
            goto normal;
        }
    }
    // Inline (normal)
    else if (d.sexp <= 1 && d.sexp >= 0 &&
             (d.frac & u64_msn) == d.frac) {
        vf_inl = 1;
        vf_exp = d.sexp + 1;
        vf_man = d.frac >> 60;
    }
    // Inline (subnormal)
    else if (d.sexp <= -1 && d.sexp >= -4 &&
             ((d.frac >> -d.sexp) & u64_msn) == (d.frac >> -d.sexp)) {
        vf_inl = 1;
        vf_exp = 0;
        vf_man = (0x10 | (d.frac >> 60)) >> -d.sexp;
    }
    // Normal Out-of-line
    else { normal:
        vf_inl = 0;
        /*
         * 1. omit fraction for powers of two (fraction is zero).
         * 2. omit exponent for integers (no fraction bits right of point).
         * 3. otherwise encode both exponent and fraction
         */
        if (d.sexp >= 0 && d.sexp < 64 && d.frac > 0 && (d.frac << d.sexp) == 0) {
            size_t sh = 64 - d.sexp;
            vf_exp = 0;
            vw_man = (d.frac >> sh) | (u64_msb >> (sh - 1));
            vf_man = vf8_le_ber_integer_u64_length(&vw_man);
        }
        else if (d.frac == 0) {
            vw_man = 0;
            vw_exp = d.sexp;
            vf_exp = vf8_le_ber_integer_s64_length(&vw_exp);
        }
        else {
            size_t sh = ctz(d.frac);
            vw_man = (d.frac >> sh) | (u64_msb >> (sh - 1));
            vw_exp = d.sexp;
            vf_exp = vf8_le_ber_integer_s64_length(&vw_exp);
            vf_man = vf8_le_ber_integer_u64_length(&vw_man);
        }
        /* vf_exp and vf_man contain length of exponent and fraction in bytes */
    }

    pre = (vf_inl << 7) | (vf_sgn << 6) | (vf_exp << 4) | vf_man;
    if (vf8_buf_write_i8(buf, pre) != 1) {
        goto err;
    }

    if (!vf_inl) {
        if (vf_exp && vf8_le_ber_integer_s64_write(buf, vf_exp, &vw_exp) < 0) {
            goto err;
        }
        if (vf_man && vf8_le_ber_integer_u64_write(buf, vf_man, &vw_man) < 0) {
            goto err;
        }
    }

#if DEBUG_ENCODING
    _vf8_debug_pack(v, pre, d.sexp, d.frac, vw_exp, vw_man);
#endif

    return 0;
err:
    return -1;
}
