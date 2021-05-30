#undef NDEBUG
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cmath>
#include <chrono>

#include "vf128.h"

using namespace std::chrono;

typedef time_point<high_resolution_clock> time_type;

const char pi_str[] = "3.141592653589793";
const unsigned char pi_asn[] = { 0x09, 0x09, 0x80, 0xD0, 0x03, 0x24, 0x3F, 0x6A, 0x88, 0x85, 0xA3 };

const unsigned long long i13 = 72057594037927935;
const char i13_str[] = "72057594037927935";
const unsigned char i13_asn[] = { 0x02, 0x08, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

const unsigned long long i12 = 18014398509481984;
const unsigned char i12_leb[] = { 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x20 };
const unsigned char i12_vlu[] = { 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40 };

const unsigned char pi_vf8[] = { 0x17, 0x01, 0xA3, 0x85, 0x88, 0x6A, 0x3F, 0x24, 0x03 };

const size_t count = 100000000;

static const char* format_unit(size_t count)
{
    static char buf[32];
    if (count % 1000000000 == 0) {
        snprintf(buf, sizeof(buf), "%zuG", count / 1000000000);
    } else if (count % 1000000 == 0) {
        snprintf(buf, sizeof(buf), "%zuM", count / 1000000);
    } else if (count % 1000 == 0) {
        snprintf(buf, sizeof(buf), "%zuK", count / 1000);
    } else {
        snprintf(buf, sizeof(buf), "%zu", count);
    }
    return buf;
}

static const char* format_comma(size_t count)
{
    static char buf[32];
    char buf1[32];

    snprintf(buf1, sizeof(buf1), "%zu", count);

    size_t l = strlen(buf1), i = 0, j = 0;
    for (; i < l; i++, j++) {
        buf[j] = buf1[i];
        if ((l-i-1) % 3 == 0 && i != l -1) {
            buf[++j] = ',';
        }
    }
    buf[j] = '\0';

    return buf;
}

static void bench_ascii_strtod()
{
    double f;
    auto st = high_resolution_clock::now();
    for (size_t i = 0; i < count; i++) {
        f = strtod(pi_str, NULL);
    }
    auto et = high_resolution_clock::now();

    assert(fabs(f - 3.141592) < 0.0001);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    printf("f64-strtod-base10      : %s ops in %5.2f sec, %6.2f ns, %11s ops/sec, %8.3f MiB/sec\n",
        format_unit(count), t / 1e9, t / count, format_comma(count * (1e9 / t)),
        count * (1e9 / t) * 8.0 / (1024*1024));
}

static void bench_asn1_read_byptr_real()
{
    double f;
    vf8_buf *buf = vf8_buf_new(128);
    vf8_buf_write_bytes(buf, (const char*)pi_asn, sizeof(pi_asn));

    auto st = high_resolution_clock::now();
    for (size_t i = 0; i < count; i++) {
        vf8_buf_reset(buf);
        assert(!vf8_asn1_der_real_f64_read(buf, asn1_tag_real, &f));
    }
    auto et = high_resolution_clock::now();

    assert(fabs(f - 3.141592) < 0.0001);
    vf8_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    printf("f64-asn.1-read-byptr   : %s ops in %5.2f sec, %6.2f ns, %11s ops/sec, %8.3f MiB/sec\n",
        format_unit(count), t / 1e9, t / count, format_comma(count * (1e9 / t)),
        count * (1e9 / t) * 8.0 / (1024*1024));
}

static void bench_asn1_read_byval_real()
{
    f64_result r;
    vf8_buf *buf = vf8_buf_new(128);
    vf8_buf_write_bytes(buf, (const char*)pi_asn, sizeof(pi_asn));

    auto st = high_resolution_clock::now();
    for (size_t i = 0; i < count; i++) {
        vf8_buf_reset(buf);
        r = vf8_asn1_der_real_f64_read_byval(buf, asn1_tag_real);
        assert(!r.error);
    }
    auto et = high_resolution_clock::now();

    assert(fabs(r.value - 3.141592) < 0.0001);
    vf8_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    printf("f64-asn.1-read-byval   : %s ops in %5.2f sec, %6.2f ns, %11s ops/sec, %8.3f MiB/sec\n",
        format_unit(count), t / 1e9, t / count, format_comma(count * (1e9 / t)),
        count * (1e9 / t) * 8.0 / (1024*1024));
}

static void bench_asn1_write_byptr_real()
{
    double f = 3.141592653589793;
    vf8_buf *buf = vf8_buf_new(128);

    auto st = high_resolution_clock::now();
    for (size_t i = 0; i < count; i++) {
        vf8_buf_reset(buf);
        assert(!vf8_asn1_der_real_f64_write(buf, asn1_tag_real, &f));
    }
    auto et = high_resolution_clock::now();

    vf8_buf_reset(buf);
    vf8_asn1_der_real_f64_read(buf, asn1_tag_real, &f);
    assert(fabs(f - 3.141592) < 0.0001);
    vf8_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    printf("f64-asn.1-write-byptr  : %s ops in %5.2f sec, %6.2f ns, %11s ops/sec, %8.3f MiB/sec\n",
        format_unit(count), t / 1e9, t / count, format_comma(count * (1e9 / t)),
        count * (1e9 / t) * 8.0 / (1024*1024));
}

static void bench_asn1_write_byval_real()
{
    f64_result r;
    double f = 3.141592653589793;
    vf8_buf *buf = vf8_buf_new(128);

    auto st = high_resolution_clock::now();
    for (size_t i = 0; i < count; i++) {
        vf8_buf_reset(buf);
        assert(!vf8_asn1_der_real_f64_write_byval(buf, asn1_tag_real, f));
    }
    auto et = high_resolution_clock::now();

    vf8_buf_reset(buf);
    r = vf8_asn1_der_real_f64_read_byval(buf, asn1_tag_real);
    assert(fabs(r.value - 3.141592) < 0.0001);
    vf8_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    printf("f64-asn.1-write-byval  : %s ops in %5.2f sec, %6.2f ns, %11s ops/sec, %8.3f MiB/sec\n",
        format_unit(count), t / 1e9, t / count, format_comma(count * (1e9 / t)),
        count * (1e9 / t) * 8.0 / (1024*1024));
}

static void bench_vf8_read_byptr_real()
{
    double f;
    vf8_buf *buf = vf8_buf_new(128);
    vf8_buf_write_bytes(buf, (const char*)pi_vf8, sizeof(pi_vf8));

    auto st = high_resolution_clock::now();
    for (size_t i = 0; i < count; i++) {
        vf8_buf_reset(buf);
        assert(!vf8_f64_read(buf, &f));
    }
    auto et = high_resolution_clock::now();

    assert(fabs(f - 3.141592) < 0.0001);
    vf8_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    printf("f64-vf128-read-byptr   : %s ops in %5.2f sec, %6.2f ns, %11s ops/sec, %8.3f MiB/sec\n",
        format_unit(count), t / 1e9, t / count, format_comma(count * (1e9 / t)),
        count * (1e9 / t) * 8.0 / (1024*1024));
}

static void bench_vf8_read_byval_real()
{
    f64_result r;
    double f;
    vf8_buf *buf = vf8_buf_new(128);
    vf8_buf_write_bytes(buf, (const char*)pi_vf8, sizeof(pi_vf8));

    auto st = high_resolution_clock::now();
    for (size_t i = 0; i < count; i++) {
        vf8_buf_reset(buf);
        r = vf8_f64_read_byval(buf);
        assert(!r.error);
    }
    auto et = high_resolution_clock::now();

    assert(fabs(r.value - 3.141592) < 0.0001);
    vf8_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    printf("f64-vf128-read-byval   : %s ops in %5.2f sec, %6.2f ns, %11s ops/sec, %8.3f MiB/sec\n",
        format_unit(count), t / 1e9, t / count, format_comma(count * (1e9 / t)),
        count * (1e9 / t) * 8.0 / (1024*1024));
}

static void bench_vf8_write_byptr_real()
{
    double f = 3.141592653589793;
    vf8_buf *buf = vf8_buf_new(128);

    auto st = high_resolution_clock::now();
    for (size_t i = 0; i < count; i++) {
        vf8_buf_reset(buf);
        assert(!vf8_f64_write(buf, &f));
    }
    auto et = high_resolution_clock::now();

    vf8_buf_reset(buf);
    vf8_f64_read(buf, &f);
    assert(fabs(f - 3.141592) < 0.0001);
    vf8_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    printf("f64-vf128-write-byptr  : %s ops in %5.2f sec, %6.2f ns, %11s ops/sec, %8.3f MiB/sec\n",
        format_unit(count), t / 1e9, t / count, format_comma(count * (1e9 / t)),
        count * (1e9 / t) * 8.0 / (1024*1024));
}

static void bench_vf8_write_byval_real()
{
    f64_result r;
    double f = 3.141592653589793;
    vf8_buf *buf = vf8_buf_new(128);

    auto st = high_resolution_clock::now();
    for (size_t i = 0; i < count; i++) {
        vf8_buf_reset(buf);
        assert(!vf8_f64_write_byval(buf, f));
    }
    auto et = high_resolution_clock::now();

    vf8_buf_reset(buf);
    r = vf8_f64_read_byval(buf);
    assert(fabs(r.value - 3.141592) < 0.0001);
    vf8_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    printf("f64-vf128-write-byval  : %s ops in %5.2f sec, %6.2f ns, %11s ops/sec, %8.3f MiB/sec\n",
        format_unit(count), t / 1e9, t / count, format_comma(count * (1e9 / t)),
        count * (1e9 / t) * 8.0 / (1024*1024));
}

static void bench_ascii_strtoull()
{
    unsigned long long d;
    auto st = high_resolution_clock::now();
    for (size_t i = 0; i < count; i++) {
        d = strtoull(i13_str, NULL, 10);
    }
    auto et = high_resolution_clock::now();

    assert(d == i13);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    printf("u64-strtoull-base10    : %s ops in %5.2f sec, %6.2f ns, %11s ops/sec, %8.3f MiB/sec\n",
        format_unit(count), t / 1e9, t / count, format_comma(count * (1e9 / t)),
        count * (1e9 / t) * 8.0 / (1024*1024));
}

static void bench_asn1_read_byptr_integer()
{
    unsigned long long d;
    vf8_buf *buf = vf8_buf_new(128);
    vf8_buf_write_bytes(buf, (const char*)i13_asn, sizeof(i13_asn));

    auto st = high_resolution_clock::now();
    for (size_t i = 0; i < count; i++) {
        vf8_buf_reset(buf);
        assert(!vf8_asn1_der_integer_u64_read(buf, asn1_tag_integer, &d));
    }
    auto et = high_resolution_clock::now();

    assert(d == i13);
    vf8_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    printf("u64-asn1.1-read-byptr  : %s ops in %5.2f sec, %6.2f ns, %11s ops/sec, %8.3f MiB/sec\n",
        format_unit(count), t / 1e9, t / count, format_comma(count * (1e9 / t)),
        count * (1e9 / t) * 8.0 / (1024*1024));
}

static void bench_asn1_read_byval_integer()
{
    u64_result r;
    vf8_buf *buf = vf8_buf_new(128);
    vf8_buf_write_bytes(buf, (const char*)i13_asn, sizeof(i13_asn));

    auto st = high_resolution_clock::now();
    for (size_t i = 0; i < count; i++) {
        vf8_buf_reset(buf);
        r = vf8_asn1_der_integer_u64_read_byval(buf, asn1_tag_integer);
        assert(!r.error);
    }
    auto et = high_resolution_clock::now();

    assert(r.value == i13);
    vf8_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    printf("u64-asn1.1-read-byval  : %s ops in %5.2f sec, %6.2f ns, %11s ops/sec, %8.3f MiB/sec\n",
        format_unit(count), t / 1e9, t / count, format_comma(count * (1e9 / t)),
        count * (1e9 / t) * 8.0 / (1024*1024));
}

static void bench_asn1_write_byptr_integer()
{
    unsigned long long d = i12;
    vf8_buf *buf = vf8_buf_new(128);

    auto st = high_resolution_clock::now();
    for (size_t i = 0; i < count; i++) {
        vf8_buf_reset(buf);
        assert(!vf8_asn1_der_integer_u64_write(buf, asn1_tag_integer, &d));
    }
    auto et = high_resolution_clock::now();

    vf8_buf_reset(buf);
    vf8_asn1_der_integer_u64_read(buf, asn1_tag_integer, &d);
    assert(d == i12);
    vf8_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    printf("u64-asn1.1-write-byptr : %s ops in %5.2f sec, %6.2f ns, %11s ops/sec, %8.3f MiB/sec\n",
        format_unit(count), t / 1e9, t / count, format_comma(count * (1e9 / t)),
        count * (1e9 / t) * 8.0 / (1024*1024));
}

static void bench_asn1_write_byval_integer()
{
    u64_result r;
    unsigned long long d = i12;
    vf8_buf *buf = vf8_buf_new(128);

    auto st = high_resolution_clock::now();
    for (size_t i = 0; i < count; i++) {
        vf8_buf_reset(buf);
        assert(!vf8_asn1_der_integer_u64_write_byval(buf, asn1_tag_integer, d));
    }
    auto et = high_resolution_clock::now();

    vf8_buf_reset(buf);
    r = vf8_asn1_der_integer_u64_read_byval(buf, asn1_tag_integer);
    assert(r.value == i12);
    vf8_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    printf("u64-asn1.1-write-byval : %s ops in %5.2f sec, %6.2f ns, %11s ops/sec, %8.3f MiB/sec\n",
        format_unit(count), t / 1e9, t / count, format_comma(count * (1e9 / t)),
        count * (1e9 / t) * 8.0 / (1024*1024));
}

static void bench_leb_read_byptr_integer()
{
    unsigned long long d;
    vf8_buf *buf = vf8_buf_new(128);
    vf8_buf_write_bytes(buf, (const char*)i12_leb, sizeof(i12_leb));

    auto st = high_resolution_clock::now();
    for (size_t i = 0; i < count; i++) {
        vf8_buf_reset(buf);
        assert(!leb_u64_read(buf, &d));
    }
    auto et = high_resolution_clock::now();

    assert(d == i12);
    vf8_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    printf("u64-leb128-read-byptr  : %s ops in %5.2f sec, %6.2f ns, %11s ops/sec, %8.3f MiB/sec\n",
        format_unit(count), t / 1e9, t / count, format_comma(count * (1e9 / t)),
        count * (1e9 / t) * 8.0 / (1024*1024));
}

static void bench_leb_read_byval_integer()
{
    u64_result r;
    vf8_buf *buf = vf8_buf_new(128);
    vf8_buf_write_bytes(buf, (const char*)i12_leb, sizeof(i12_leb));

    auto st = high_resolution_clock::now();
    for (size_t i = 0; i < count; i++) {
        vf8_buf_reset(buf);
        r = leb_u64_read_byval(buf);
        assert(!r.error);
    }
    auto et = high_resolution_clock::now();

    vf8_buf_destroy(buf);
    assert(r.value == i12);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    printf("u64-leb128-read-byval  : %s ops in %5.2f sec, %6.2f ns, %11s ops/sec, %8.3f MiB/sec\n",
        format_unit(count), t / 1e9, t / count, format_comma(count * (1e9 / t)),
        count * (1e9 / t) * 8.0 / (1024*1024));
}

static void bench_leb_write_byptr_integer()
{
    unsigned long long d = i12;
    vf8_buf *buf = vf8_buf_new(128);

    auto st = high_resolution_clock::now();
    for (size_t i = 0; i < count; i++) {
        vf8_buf_reset(buf);
        assert(!leb_u64_write(buf, &d));
    }
    auto et = high_resolution_clock::now();

    vf8_buf_reset(buf);
    leb_u64_read(buf, &d);
    assert(d == i12);
    vf8_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    printf("u64-leb128-write-byptr : %s ops in %5.2f sec, %6.2f ns, %11s ops/sec, %8.3f MiB/sec\n",
        format_unit(count), t / 1e9, t / count, format_comma(count * (1e9 / t)),
        count * (1e9 / t) * 8.0 / (1024*1024));
}

static void bench_leb_write_byval_integer()
{
    unsigned long long d = i12;
    vf8_buf *buf = vf8_buf_new(128);

    auto st = high_resolution_clock::now();
    for (size_t i = 0; i < count; i++) {
        vf8_buf_reset(buf);
        assert(!leb_u64_write_byval(buf, d));
    }
    auto et = high_resolution_clock::now();

    vf8_buf_reset(buf);
    d = leb_u64_read_byval(buf).value;
    assert(d == i12);
    vf8_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    printf("u64-leb128-write-byval : %s ops in %5.2f sec, %6.2f ns, %11s ops/sec, %8.3f MiB/sec\n",
        format_unit(count), t / 1e9, t / count, format_comma(count * (1e9 / t)),
        count * (1e9 / t) * 8.0 / (1024*1024));
}

static void bench_vlu_read_byptr_integer()
{
    unsigned long long d;
    vf8_buf *buf = vf8_buf_new(128);
    vf8_buf_write_bytes(buf, (const char*)i12_vlu, sizeof(i12_vlu));

    auto st = high_resolution_clock::now();
    for (size_t i = 0; i < count; i++) {
        vf8_buf_reset(buf);
        assert(!vlu_u64_read(buf, &d));
    }
    auto et = high_resolution_clock::now();

    assert(d == i12);
    vf8_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    printf("u64-vlu8-read-byptr    : %s ops in %5.2f sec, %6.2f ns, %11s ops/sec, %8.3f MiB/sec\n",
        format_unit(count), t / 1e9, t / count, format_comma(count * (1e9 / t)),
        count * (1e9 / t) * 8.0 / (1024*1024));
}

static void bench_vlu_read_byval_integer()
{
    u64_result r;
    vf8_buf *buf = vf8_buf_new(128);
    vf8_buf_write_bytes(buf, (const char*)i12_vlu, sizeof(i12_vlu));

    auto st = high_resolution_clock::now();
    for (size_t i = 0; i < count; i++) {
        vf8_buf_reset(buf);
        r = vlu_u64_read_byval(buf);
        assert(!r.error);
    }
    auto et = high_resolution_clock::now();

    assert(r.value == i12);
    vf8_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    printf("u64-vlu8-read-byval    : %s ops in %5.2f sec, %6.2f ns, %11s ops/sec, %8.3f MiB/sec\n",
        format_unit(count), t / 1e9, t / count, format_comma(count * (1e9 / t)),
        count * (1e9 / t) * 8.0 / (1024*1024));
}

static void bench_vlu_write_byptr_integer()
{
    unsigned long long d = i12;
    vf8_buf *buf = vf8_buf_new(128);

    auto st = high_resolution_clock::now();
    for (size_t i = 0; i < count; i++) {
        vf8_buf_reset(buf);
        assert(!vlu_u64_write(buf, &d));
    }
    auto et = high_resolution_clock::now();

    vf8_buf_reset(buf);
    vlu_u64_read(buf, &d);
    assert(d == i12);
    vf8_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    printf("u64-vlu8-write-byptr   : %s ops in %5.2f sec, %6.2f ns, %11s ops/sec, %8.3f MiB/sec\n",
        format_unit(count), t / 1e9, t / count, format_comma(count * (1e9 / t)),
        count * (1e9 / t) * 8.0 / (1024*1024));
}

static void bench_vlu_write_byval_integer()
{
    unsigned long long d = i12;
    vf8_buf *buf = vf8_buf_new(128);

    auto st = high_resolution_clock::now();
    for (size_t i = 0; i < count; i++) {
        vf8_buf_reset(buf);
        assert(!vlu_u64_write_byval(buf, d));
    }
    auto et = high_resolution_clock::now();

    vf8_buf_reset(buf);
    d = vlu_u64_read_byval(buf).value;
    assert(d == i12);
    vf8_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    printf("u64-vlu8-write-byval   : %s ops in %5.2f sec, %6.2f ns, %11s ops/sec, %8.3f MiB/sec\n",
        format_unit(count), t / 1e9, t / count, format_comma(count * (1e9 / t)),
        count * (1e9 / t) * 8.0 / (1024*1024));
}

int main(int argc, char **argv)
{
    bench_ascii_strtod();
    bench_asn1_read_byptr_real();
    bench_asn1_read_byval_real();
    bench_asn1_write_byptr_real();
    bench_asn1_write_byval_real();
    bench_vf8_read_byptr_real();
    bench_vf8_read_byval_real();
    bench_vf8_write_byptr_real();
    bench_vf8_write_byval_real();
    bench_ascii_strtoull();
    bench_asn1_read_byptr_integer();
    bench_asn1_read_byval_integer();
    bench_asn1_write_byptr_integer();
    bench_asn1_write_byval_integer();
    bench_leb_read_byptr_integer();
    bench_leb_read_byval_integer();
    bench_leb_write_byptr_integer();
    bench_leb_write_byval_integer();
    bench_vlu_read_byptr_integer();
    bench_vlu_read_byval_integer();
    bench_vlu_write_byptr_integer();
    bench_vlu_write_byval_integer();
}
