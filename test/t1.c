#undef NDEBUG
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "vf128.h"

const char* ber_real_fmt = "\nASN.1 Real(%.16g)\n";
const char* vf64_fmt = "\nvf64 out(%.16g) in(%.16g)\n";
const char* vf32_fmt = "\nvf32 out(%.8g) in(%.8g)\n";
const char* leb_fmt = "\nLEB out(%llu) in(%llu)\n";
const char* vlu_fmt = "\nVLU out(%llu) in(%llu)\n";

const float pi_f32 = 3.141592f;
const double pi_f64 = 3.141592653589793;
const unsigned char pi_asn[] = { 0x80, 0xD0, 0x03, 0x24, 0x3F, 0x6A, 0x88, 0x85, 0xA3 };

void test_ber_pi()
{
    double f;
    vf_buf *buf = vf_buf_new(128);
    size_t len = vf_asn1_ber_real_f64_length(&pi_f64);
    vf_asn1_ber_real_f64_write(buf, len, &pi_f64);
    vf_buf_reset(buf);
    vf_asn1_ber_real_f64_read(buf, len, &f);
    printf(ber_real_fmt, (double)f);
    vf_buf_dump(buf);
    vf_buf_destroy(buf);
}

void test_vf64(double f)
{
    double r;
    vf_buf *buf = vf_buf_new(128);
    assert(!vf_f64_write(buf, &f));
    vf_buf_reset(buf);
    vf_f64_read(buf, &r);
    printf(vf64_fmt, f, r);
    vf_buf_dump(buf);
    assert(isnan(f) ? isnan(r) : f == r);
    vf_buf_destroy(buf);
}

void test_vf64_loop()
{
    test_vf64(pi_f64);
    test_vf64(_f64_inf());
    test_vf64(_f64_nan());
    test_vf64(_f64_snan());
    test_vf64(-_f64_inf());
    test_vf64(-_f64_nan());
    test_vf64(-_f64_snan());
    union { u64 u; f64 d; } subnormal1 = { 0x1ull };
    union { u64 u; f64 d; } subnormal2 = { 0x1ull };
    test_vf64(subnormal1.d);
    test_vf64(subnormal2.d);
    for (double i = -3.875; i <= 3.875; i += (i < -0.5 || i >= 0.5) ? 0.125 : 0.0625) {
        test_vf64(i);
    }
    for (double i = 1; i <= 16; i += 0.5) {
        test_vf64(i);
    }
    for (double i = 511; i <= 513; i += 0.5) {
        test_vf64(i);
    }
    for (double i = 65534; i <= 65536; i += 0.5) {
        test_vf64(i);
    }
    for (int i = 1; i <= 10; i++) {
        test_vf64(1.0/(1 << i));
    }
    for (double i = 0.001; i < 0.902; i += 0.050) {
        test_vf64(i);
    }
    test_vf64(0.000001);
}

void test_vf32(float f)
{
    float r;
    vf_buf *buf = vf_buf_new(128);
    assert(!vf_f32_write(buf, &f));
    vf_buf_reset(buf);
    vf_f32_read(buf, &r);
    printf(vf32_fmt, f, r);
    vf_buf_dump(buf);
    assert(isnan(f) ? isnan(r) : f == r);
    vf_buf_destroy(buf);
}

void test_vf32_loop()
{
    test_vf32(pi_f32);
    test_vf32(_f32_inf());
    test_vf32(_f32_nan());
    test_vf32(_f32_snan());
    test_vf32(-_f32_inf());
    test_vf32(-_f32_nan());
    test_vf32(-_f32_snan());
    union { u32 u; f32 d; } subnormal1 = { 0x1ull };
    union { u32 u; f32 d; } subnormal2 = { 0x1ull };
    test_vf32(subnormal1.d);
    test_vf32(subnormal2.d);
    for (float i = -3.875f; i <= 3.875f; i += (i < -0.5f || i >= 0.5f) ? 0.125f : 0.0625f) {
        test_vf32(i);
    }
    for (float i = 1; i <= 16; i += 0.5f) {
        test_vf32(i);
    }
    for (float i = 511; i <= 513; i += 0.5f) {
        test_vf32(i);
    }
    for (float i = 65534; i <= 65536; i += 0.5f) {
        test_vf32(i);
    }
    for (int i = 1; i <= 10; i++) {
        test_vf32(1.0f/(1 << i));
    }
    for (float i = 0.001f; i < 0.902f; i += 0.050f) {
        test_vf32(i);
    }
    test_vf32(0.000001f);
}

int main(int argc, const char **argv)
{
    test_ber_pi();
    test_vf64_loop();
    test_vf32_loop();
}
