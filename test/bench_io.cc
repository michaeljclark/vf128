#undef NDEBUG
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cmath>
#include <chrono>

#include "vf128.h"

#ifdef _WIN32
#include <Windows.h>
#include <synchapi.h>
#else
#include <time.h>
#endif

using namespace std::chrono;

typedef signed long long llong;
typedef unsigned long long ullong;

static void _millisleep(llong sleep_ms)
{
#ifdef _WIN32
    HANDLE hTimer;
    LARGE_INTEGER liDueTime;
    liDueTime.QuadPart = -10000LL * sleep_ms;
    assert((hTimer = CreateWaitableTimer(NULL, TRUE, NULL)));
    assert(SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0));
    assert(WaitForSingleObject(hTimer, INFINITE) == WAIT_OBJECT_0);
    CloseHandle(hTimer);
#else
    struct timespec ts = {
        (time_t)(sleep_ms / 1000),
        (long)((sleep_ms * 1000000ll) % 1000000000ll)
    };
    nanosleep(&ts, nullptr);
#endif
}

char pi_str[] = "3.141592653589793";
char pi_strf[] = "3.141592";
unsigned char pi_asn[] = { 0x09, 0x09, 0x80, 0xD0, 0x03, 0x24, 0x3F, 0x6A, 0x88, 0x85, 0xA3 };
unsigned long long i17 = 72057594037927935;
char i17_str[] = "72057594037927935";
unsigned int i10 = 2147483648;
char i10_str[] = "2147483648";
unsigned char i17_asn[] = { 0x02, 0x08, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
unsigned long long i12 = 18014398509481984;
unsigned char i12_leb[] = { 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x20 };
unsigned char i12_vlu[] = { 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40 };
unsigned char pi_vf8[] = { 0x17, 0x01, 0xA3, 0x85, 0x88, 0x6A, 0x3F, 0x24, 0x03 };

struct bench_result { const char *name; llong count; double t; llong size; };

static bench_result bench_ascii_strtod(llong count)
{
    double f;
    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        f = strtod(pi_str, NULL);
    }
    auto et = high_resolution_clock::now();

    assert(fabs(f - 3.141592) < 0.0001);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "f64-read-strtod-dec", count, t, 8 * count };
}


static bench_result bench_ascii_strtof(llong count)
{
    float f;
    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        f = strtof(pi_strf, NULL);
    }
    auto et = high_resolution_clock::now();

    assert(fabsf(f - 3.141592f) < 0.0001f);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "f32-read-strtof-dec", count, t, 4 * count };
}

static bench_result bench_ascii_snprintf_f(llong count)
{
    char buf[32];
    const float pi = 3.141592f;
    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        assert(snprintf(buf, sizeof(buf), "%.8lf", pi) > 0);
    }
    auto et = high_resolution_clock::now();

    assert(strncmp(buf,"3.141592", 8) == 0);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "f32-write-snprintf-dec", count, t, 4 * count };
}


static bench_result bench_ascii_snprintf_d(llong count)
{
    char buf[32];
    const double pi = 3.141592653589793;
    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        assert(snprintf(buf, sizeof(buf), "%.16lf", pi) > 0);
    }
    auto et = high_resolution_clock::now();

    assert(strncmp(buf,"3.141592653589793", 17) == 0);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "f64-write-snprintf-dec", count, t, 8 * count };
}

static bench_result bench_asn1_read_byptr_real(llong count)
{
    double f;
    vf_buf *buf = vf_buf_new(128);
    vf_buf_write_bytes(buf, (const char*)pi_asn, sizeof(pi_asn));

    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        vf_buf_reset(buf);
        assert(!vf_asn1_der_real_f64_read(buf, asn1_tag_real, &f));
    }
    auto et = high_resolution_clock::now();

    assert(fabs(f - 3.141592) < 0.0001);
    vf_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "f64-asn.1-read-byptr", count, t, 8 * count };
}

static bench_result bench_asn1_read_byval_real(llong count)
{
    f64_result r;
    vf_buf *buf = vf_buf_new(128);
    vf_buf_write_bytes(buf, (const char*)pi_asn, sizeof(pi_asn));

    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        vf_buf_reset(buf);
        r = vf_asn1_der_real_f64_read_byval(buf, asn1_tag_real);
        assert(!r.error);
    }
    auto et = high_resolution_clock::now();

    assert(fabs(r.value - 3.141592) < 0.0001);
    vf_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "f64-asn.1-read-byval", count, t, 8 * count };
}

static bench_result bench_asn1_write_byptr_real(llong count)
{
    double f = 3.141592653589793;
    vf_buf *buf = vf_buf_new(128);

    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        vf_buf_reset(buf);
        assert(!vf_asn1_der_real_f64_write(buf, asn1_tag_real, &f));
    }
    auto et = high_resolution_clock::now();

    vf_buf_reset(buf);
    vf_asn1_der_real_f64_read(buf, asn1_tag_real, &f);
    assert(fabs(f - 3.141592) < 0.0001);
    vf_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "f64-asn.1-write-byptr", count, t, 8 * count };
}

static bench_result bench_asn1_write_byval_real(llong count)
{
    f64_result r;
    double f = 3.141592653589793;
    vf_buf *buf = vf_buf_new(128);

    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        vf_buf_reset(buf);
        assert(!vf_asn1_der_real_f64_write_byval(buf, asn1_tag_real, f));
    }
    auto et = high_resolution_clock::now();

    vf_buf_reset(buf);
    r = vf_asn1_der_real_f64_read_byval(buf, asn1_tag_real);
    assert(fabs(r.value - 3.141592) < 0.0001);
    vf_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "f64-asn.1-write-byval", count, t, 8 * count };
}

static bench_result bench_vf64_read_byptr_real(llong count)
{
    double f;
    vf_buf *buf = vf_buf_new(128);
    vf_buf_write_bytes(buf, (const char*)pi_vf8, sizeof(pi_vf8));

    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        vf_buf_reset(buf);
        assert(!vf_f64_read(buf, &f));
    }
    auto et = high_resolution_clock::now();

    assert(fabs(f - 3.141592) < 0.0001);
    vf_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "f64-vf128-read-byptr", count, t, 8 * count };
}

static bench_result bench_vf64_read_byval_real(llong count)
{
    f64_result r;
    vf_buf *buf = vf_buf_new(128);
    vf_buf_write_bytes(buf, (const char*)pi_vf8, sizeof(pi_vf8));

    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        vf_buf_reset(buf);
        r = vf_f64_read_byval(buf);
        assert(!r.error);
    }
    auto et = high_resolution_clock::now();

    assert(fabs(r.value - 3.141592) < 0.0001);
    vf_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "f64-vf128-read-byval", count, t, 8 * count };
}

static bench_result bench_vf64_write_byptr_real(llong count)
{
    double f = 3.141592653589793;
    vf_buf *buf = vf_buf_new(128);

    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        vf_buf_reset(buf);
        assert(!vf_f64_write(buf, &f));
    }
    auto et = high_resolution_clock::now();

    vf_buf_reset(buf);
    vf_f64_read(buf, &f);
    assert(fabs(f - 3.141592) < 0.0001);
    vf_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "f64-vf128-write-byptr", count, t, 8 * count };
}

static bench_result bench_vf64_write_byval_real(llong count)
{
    f64_result r;
    double f = 3.141592653589793;
    vf_buf *buf = vf_buf_new(128);

    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        vf_buf_reset(buf);
        assert(!vf_f64_write_byval(buf, f));
    }
    auto et = high_resolution_clock::now();

    vf_buf_reset(buf);
    r = vf_f64_read_byval(buf);
    assert(fabs(r.value - 3.141592) < 0.0001);
    vf_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "f64-vf128-write-byval", count, t, 8 * count };
}

static bench_result bench_vf32_read_byptr_real(llong count)
{
    float f;
    vf_buf *buf = vf_buf_new(128);
    vf_buf_write_bytes(buf, (const char*)pi_vf8, sizeof(pi_vf8));

    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        vf_buf_reset(buf);
        assert(!vf_f32_read(buf, &f));
    }
    auto et = high_resolution_clock::now();

    assert(fabs(f - 3.141592f) < 0.0001);
    vf_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "f32-vf128-read-byptr", count, t, 4 * count };
}

static bench_result bench_vf32_read_byval_real(llong count)
{
    f32_result r;
    vf_buf *buf = vf_buf_new(128);
    vf_buf_write_bytes(buf, (const char*)pi_vf8, sizeof(pi_vf8));

    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        vf_buf_reset(buf);
        r = vf_f32_read_byval(buf);
        assert(!r.error);
    }
    auto et = high_resolution_clock::now();

    assert(fabs(r.value - 3.141592f) < 0.0001);
    vf_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "f32-vf128-read-byval", count, t, 4 * count };
}

static bench_result bench_vf32_write_byptr_real(llong count)
{
    float f = 3.141592f;
    vf_buf *buf = vf_buf_new(128);

    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        vf_buf_reset(buf);
        assert(!vf_f32_write(buf, &f));
    }
    auto et = high_resolution_clock::now();

    vf_buf_reset(buf);
    vf_f32_read(buf, &f);
    assert(fabs(f - 3.141592f) < 0.0001);
    vf_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "f32-vf128-write-byptr", count, t, 4 * count };
}

static bench_result bench_vf32_write_byval_real(llong count)
{
    f32_result r;
    float f = 3.141592f;
    vf_buf *buf = vf_buf_new(128);

    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        vf_buf_reset(buf);
        assert(!vf_f32_write_byval(buf, f));
    }
    auto et = high_resolution_clock::now();

    vf_buf_reset(buf);
    r = vf_f32_read_byval(buf);
    assert(fabs(r.value - 3.141592f) < 0.0001);
    vf_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "f32-vf128-write-byval", count, t, 4 * count };
}

static bench_result bench_ascii_atoi(llong count)
{
    unsigned int d;
    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        d = atoi(i10_str);
    }
    auto et = high_resolution_clock::now();

    assert(d == i10);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "u32-read-atoi-dec", count, t, 4 * count };
}

static bench_result bench_ascii_atoll(llong count)
{
    unsigned long long d;
    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        d = atoll(i17_str);
    }
    auto et = high_resolution_clock::now();

    assert(d == i17);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "u64-read-atoll-dec", count, t, 8 * count };
}

static bench_result bench_ascii_strtoull(llong count)
{
    unsigned long long d;
    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        d = strtoull(i17_str, NULL, 10);
    }
    auto et = high_resolution_clock::now();

    assert(d == i17);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "u64-read-strtoull-dec", count, t, 8 * count };
}

static bench_result bench_ascii_snprintf_llu(llong count)
{
    char buf[32];
    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        assert(snprintf(buf, sizeof(buf), "%llu", i17) > 0);
    }
    auto et = high_resolution_clock::now();

    assert(strncmp(buf, i17_str, 17) == 0);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "u64-write-snprintf-dec", count, t, 8 * count };
}

static bench_result bench_asn1_read_byptr_integer(llong count)
{
    unsigned long long d;
    vf_buf *buf = vf_buf_new(128);
    vf_buf_write_bytes(buf, (const char*)i17_asn, sizeof(i17_asn));

    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        vf_buf_reset(buf);
        assert(!vf_asn1_der_integer_u64_read(buf, asn1_tag_integer, &d));
    }
    auto et = high_resolution_clock::now();

    assert(d == i17);
    vf_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "u64-asn1.1-read-byptr", count, t, 8 * count };
}

static bench_result bench_asn1_read_byval_integer(llong count)
{
    u64_result r;
    vf_buf *buf = vf_buf_new(128);
    vf_buf_write_bytes(buf, (const char*)i17_asn, sizeof(i17_asn));

    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        vf_buf_reset(buf);
        r = vf_asn1_der_integer_u64_read_byval(buf, asn1_tag_integer);
        assert(!r.error);
    }
    auto et = high_resolution_clock::now();

    assert(r.value == i17);
    vf_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "u64-asn1.1-read-byval", count, t, 8 * count };
}

static bench_result bench_asn1_write_byptr_integer(llong count)
{
    unsigned long long d = i12;
    vf_buf *buf = vf_buf_new(128);

    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        vf_buf_reset(buf);
        assert(!vf_asn1_der_integer_u64_write(buf, asn1_tag_integer, &d));
    }
    auto et = high_resolution_clock::now();

    vf_buf_reset(buf);
    vf_asn1_der_integer_u64_read(buf, asn1_tag_integer, &d);
    assert(d == i12);
    vf_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "u64-asn1.1-write-byptr", count, t, 8 * count };
}

static bench_result bench_asn1_write_byval_integer(llong count)
{
    u64_result r;
    unsigned long long d = i12;
    vf_buf *buf = vf_buf_new(128);

    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        vf_buf_reset(buf);
        assert(!vf_asn1_der_integer_u64_write_byval(buf, asn1_tag_integer, d));
    }
    auto et = high_resolution_clock::now();

    vf_buf_reset(buf);
    r = vf_asn1_der_integer_u64_read_byval(buf, asn1_tag_integer);
    assert(r.value == i12);
    vf_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "u64-asn1.1-write-byval", count, t, 8 * count };
}

static bench_result bench_leb_read_byptr_integer(llong count)
{
    unsigned long long d;
    vf_buf *buf = vf_buf_new(128);
    vf_buf_write_bytes(buf, (const char*)i12_leb, sizeof(i12_leb));

    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        vf_buf_reset(buf);
        assert(!leb_u64_read(buf, &d));
    }
    auto et = high_resolution_clock::now();

    assert(d == i12);
    vf_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "u64-leb128-read-byptr", count, t, 8 * count };
}

static bench_result bench_leb_read_byval_integer(llong count)
{
    u64_result r;
    vf_buf *buf = vf_buf_new(128);
    vf_buf_write_bytes(buf, (const char*)i12_leb, sizeof(i12_leb));

    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        vf_buf_reset(buf);
        r = leb_u64_read_byval(buf);
        assert(!r.error);
    }
    auto et = high_resolution_clock::now();

    vf_buf_destroy(buf);
    assert(r.value == i12);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "u64-leb128-read-byval", count, t, 8 * count };
}

static bench_result bench_leb_write_byptr_integer(llong count)
{
    unsigned long long d = i12;
    vf_buf *buf = vf_buf_new(128);

    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        vf_buf_reset(buf);
        assert(!leb_u64_write(buf, &d));
    }
    auto et = high_resolution_clock::now();

    vf_buf_reset(buf);
    leb_u64_read(buf, &d);
    assert(d == i12);
    vf_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "u64-leb128-write-byptr", count, t, 8 * count };
}

static bench_result bench_leb_write_byval_integer(llong count)
{
    unsigned long long d = i12;
    vf_buf *buf = vf_buf_new(128);

    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        vf_buf_reset(buf);
        assert(!leb_u64_write_byval(buf, d));
    }
    auto et = high_resolution_clock::now();

    vf_buf_reset(buf);
    d = leb_u64_read_byval(buf).value;
    assert(d == i12);
    vf_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "u64-leb128-write-byval", count, t, 8 * count };
}

static bench_result bench_vlu_read_byptr_integer(llong count)
{
    unsigned long long d;
    vf_buf *buf = vf_buf_new(128);
    vf_buf_write_bytes(buf, (const char*)i12_vlu, sizeof(i12_vlu));

    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        vf_buf_reset(buf);
        assert(!vlu_u64_read(buf, &d));
    }
    auto et = high_resolution_clock::now();

    assert(d == i12);
    vf_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "u64-vlu8-read-byptr", count, t, 8 * count };
}

static bench_result bench_vlu_read_byval_integer(llong count)
{
    u64_result r;
    vf_buf *buf = vf_buf_new(128);
    vf_buf_write_bytes(buf, (const char*)i12_vlu, sizeof(i12_vlu));

    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        vf_buf_reset(buf);
        r = vlu_u64_read_byval(buf);
        assert(!r.error);
    }
    auto et = high_resolution_clock::now();

    assert(r.value == i12);
    vf_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "u64-vlu8-read-byval", count, t, 8 * count };
}

static bench_result bench_vlu_write_byptr_integer(llong count)
{
    unsigned long long d = i12;
    vf_buf *buf = vf_buf_new(128);

    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        vf_buf_reset(buf);
        assert(!vlu_u64_write(buf, &d));
    }
    auto et = high_resolution_clock::now();

    vf_buf_reset(buf);
    vlu_u64_read(buf, &d);
    assert(d == i12);
    vf_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "u64-vlu8-write-byptr", count, t, 8 * count };
}

static bench_result bench_vlu_write_byval_integer(llong count)
{
    unsigned long long d = i12;
    vf_buf *buf = vf_buf_new(128);

    auto st = high_resolution_clock::now();
    for (llong i = 0; i < count; i++) {
        vf_buf_reset(buf);
        assert(!vlu_u64_write_byval(buf, d));
    }
    auto et = high_resolution_clock::now();

    vf_buf_reset(buf);
    d = vlu_u64_read_byval(buf).value;
    assert(d == i12);
    vf_buf_destroy(buf);

    double t = (double)duration_cast<nanoseconds>(et - st).count();
    return bench_result { "u64-vlu8-write-byval", count, t, 8 * count };
}

static const char* format_unit(llong count)
{
    static char buf[32];
    if (count % 1000000000 == 0) {
        snprintf(buf, sizeof(buf), "%lluG", count / 1000000000);
    } else if (count % 1000000 == 0) {
        snprintf(buf, sizeof(buf), "%lluM", count / 1000000);
    } else if (count % 1000 == 0) {
        snprintf(buf, sizeof(buf), "%lluK", count / 1000);
    } else {
        snprintf(buf, sizeof(buf), "%llu", count);
    }
    return buf;
}

static const char* format_comma(llong count)
{
    static char buf[32];
    char buf1[32];

    snprintf(buf1, sizeof(buf1), "%llu", count);

    llong l = strlen(buf1), i = 0, j = 0;
    for (; i < l; i++, j++) {
        buf[j] = buf1[i];
        if ((l-i-1) % 3 == 0 && i != l -1) {
            buf[++j] = ',';
        }
    }
    buf[j] = '\0';

    return buf;
}

static bench_result(* const benchmarks[])(llong) = {
    bench_ascii_strtod,
    bench_ascii_strtof,
    bench_ascii_snprintf_f,
    bench_ascii_snprintf_d,
    bench_asn1_read_byptr_real,
    bench_asn1_read_byval_real,
    bench_asn1_write_byptr_real,
    bench_asn1_write_byval_real,
    bench_vf32_read_byptr_real,
    bench_vf32_read_byval_real,
    bench_vf32_write_byptr_real,
    bench_vf32_write_byval_real,
    bench_vf64_read_byptr_real,
    bench_vf64_read_byval_real,
    bench_vf64_write_byptr_real,
    bench_vf64_write_byval_real,
    bench_ascii_atoi,
    bench_ascii_atoll,
    bench_ascii_strtoull,
    bench_ascii_snprintf_llu,
    bench_asn1_read_byptr_integer,
    bench_asn1_read_byval_integer,
    bench_asn1_write_byptr_integer,
    bench_asn1_write_byval_integer,
    bench_leb_read_byptr_integer,
    bench_leb_read_byval_integer,
    bench_leb_write_byptr_integer,
    bench_leb_write_byval_integer,
    bench_vlu_read_byptr_integer,
    bench_vlu_read_byval_integer,
    bench_vlu_write_byptr_integer,
    bench_vlu_write_byval_integer,
};

#define array_size(arr) ((sizeof(arr)/sizeof(arr[0])))

static void print_header(const char *prefix)
{
    printf("%s%-24s %7s %7s %7s %13s %9s\n",
        prefix,
        "benchmark",
        "count",
        "time(s)",
        "op(ns)",
        "ops/s",
        "MiB/s"
    );
}

static void print_rules(const char *prefix)
{
    printf("%s%-24s %7s %7s %7s %13s %9s\n",
        prefix,
        "------------------------",
        "-------",
        "-------",
        "-------",
        "-------------",
        "---------"
    );
}

static void print_result(const char *prefix, const char *name,
    llong count, double t, llong size)
{
    printf("%s%-24s %7s %7.2f %7.2f %13s %9.3f\n",
        prefix,
        name,
        format_unit(count),
        t / 1e9,
        t / count,
        format_comma((llong)(count * (1e9 / t))),
        size * (1e9 / t) / (1024*1024)
    );
}

static void run_benchmark(size_t n, llong repeat, llong count, llong pause_ms)
{
    double min_t = 0., max_t = 0., sum_t = 0.;
    const char* name = "";
    size_t size;
    if (repeat > 0) {
        char num[32];
        snprintf(num, sizeof(num), "  [%2zu] ", n);
        print_header(num);
        print_rules("       ");
    }
    for (llong i = 0; i < llabs(repeat); i++) {
        bench_result r = benchmarks[n](count);
        name = r.name;
        size = r.size;
        if (min_t == 0. || r.t < min_t) min_t = r.t;
        if (max_t == 0. || r.t > max_t) max_t = r.t;
        sum_t += r.t;
        if (repeat > 0) {
            char run[32];
            snprintf(run, sizeof(run), "%3llu/%-3llu", i+1, repeat);
            print_result(run, name, count, r.t, size);
        }
    }
    if (repeat > 0) {
        print_rules("       ");
        print_result("worst: ", name, count, max_t, size);
        print_result("  avg: ", name, count, sum_t / repeat, size);
        print_result(" best: ", name, count, min_t, size);
        puts("");
    } else if (llabs(repeat) >= 1) {
        char num[32];
        snprintf(num, sizeof(num), "[%2zu] ", n);
        print_result(num, name, count, min_t, size);
    }
}

#if defined(_WIN32)
# define strtok_r strtok_s
#endif

int main(int argc, char **argv)
{
    llong bench_num = -1, repeat = 1, count = 10000000, pause_ms = 0;
    if (argc != 5) {
        fprintf(stderr, "usage: %s [bench_num(,…)] [repeat] [count] [pause_ms]\n", argv[0]);
        fprintf(stderr, "\ne.g.   %s -1 -10 10000000 1000\n", argv[0]);
        exit(0);
    }
    if (argc > 1) {
        bench_num = atoll(argv[1]);
    }
    if (argc > 2) {
        repeat = atoi(argv[2]);
    }
    if (argc > 3) {
        count = atoll(argv[3]);
    }
    if (argc > 4) {
        pause_ms = atoll(argv[4]);
    }
    if (repeat < 0) {
        print_header("     ");
        print_rules("     ");
    }
    if (bench_num == -1) {
        for (llong n = 0; n < array_size(benchmarks); n++) {
            if (pause_ms > 0 && n > 0) _millisleep(pause_ms);
            run_benchmark(n, repeat, count, pause_ms);
        }
    } else {
        char *save, *comp = strtok_r(argv[1], ",", &save);
        while (comp) {
            bench_num = atoll(comp);
            if (bench_num > 0 && bench_num < array_size(benchmarks)) {
                run_benchmark(bench_num, repeat, count, pause_ms);
            }
            comp = strtok_r(nullptr, ",", &save);
        }
    }
}
