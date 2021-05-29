#undef NDEBUG
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "vf128.h"

const char* ber_real_fmt = "\nASN.1 Real(%.16g)\n";
const char* ber_vf128_fmt = "\nvf128 out(%.16g) in(%.16g)\n";

const double pi_dbl = 3.141592653589793;
const char pi_str[] = "3.141592653589793";
const unsigned char pi_asn[] = { 0x80, 0xD0, 0x03, 0x24, 0x3F, 0x6A, 0x88, 0x85, 0xA3 };

void test_ber_pi()
{
    double f;
    vf8_buf *buf = vf8_buf_new(128);
    size_t len = vf8_asn1_ber_real_f64_length(&pi_dbl);
    vf8_asn1_ber_real_f64_write(buf, len, &pi_dbl);
    vf8_buf_reset(buf);
    vf8_asn1_ber_real_f64_read(buf, len, &f);
    printf(ber_real_fmt, (double)f);
    vf8_buf_dump(buf);
    vf8_buf_destroy(buf);
}

void test_vf8(double f)
{
    double r;
    vf8_buf *buf = vf8_buf_new(128);
    assert(!vf8_f64_write(buf, &f));
    vf8_buf_reset(buf);
    vf8_f64_read(buf, &r);
    printf(ber_vf128_fmt, f, r);
    vf8_buf_dump(buf);
    assert(isnan(f) ? isnan(r) : f == r);
    vf8_buf_destroy(buf);
}

void test_vf8_loop()
{
    for (double i = -3.875; i <= 3.875; i += (i < -0.5 || i >= 0.5) ? 0.125 : 0.0625) {
        test_vf8(i);
    }
}

void test_vf8_adhoc()
{
    test_vf8(pi_dbl);
    test_vf8(_f64_inf());
    test_vf8(_f64_nan());
    test_vf8(_f64_snan());
    test_vf8(-_f64_inf());
    test_vf8(-_f64_nan());
    test_vf8(-_f64_snan());
    for (double i = 1; i <= 16; i += 0.5) {
        test_vf8(i);
    }
    for (double i = 511; i <= 513; i += 0.5) {
        test_vf8(i);
    }
    for (double i = 65534; i <= 65536; i += 0.5) {
        test_vf8(i);
    }
    for (int i = 1; i <= 10; i++) {
        test_vf8(1.0/(1 << i));
    }
}

int main(int argc, const char **argv)
{
    test_ber_pi();
    test_vf8_loop();
    test_vf8_adhoc();
}
