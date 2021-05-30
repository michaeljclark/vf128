#undef NDEBUG
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "vf128.h"

const char* ber_real_fmt = "\nASN.1 Real(%.16g)\n";
const char* vf128_fmt = "\nvf128 out(%.16g) in(%.16g)\n";
const char* leb_fmt = "\nLEB out(%llu) in(%llu)\n";
const char* vlu_fmt = "\nVLU out(%llu) in(%llu)\n";

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
    printf(vf128_fmt, f, r);
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

void test_leb(u64 val)
{
    u64 val2;
    vf8_buf *buf = vf8_buf_new(128);
    leb_u64_write(buf, &val);
    vf8_buf_reset(buf);
    leb_u64_read(buf, &val2);
    printf(leb_fmt, val, val2);
    vf8_buf_dump(buf);
    vf8_buf_destroy(buf);
}

void test_vlu(u64 val)
{
    u64 val2;
    vf8_buf *buf = vf8_buf_new(128);
    vlu_u64_write(buf, &val);
    vf8_buf_reset(buf);
    vlu_u64_read(buf, &val2);
    printf(vlu_fmt, val, val2);
    vf8_buf_dump(buf);
    vf8_buf_destroy(buf);
}

void test_vlu_byval(u64 val)
{
    u64 val2;
    vf8_buf *buf = vf8_buf_new(128);
    vlu_u64_write(buf, &val);
    vf8_buf_reset(buf);
    val2 = vlu_u64_read_byval(buf).value;
    printf(vlu_fmt, val, val2);
    vf8_buf_dump(buf);
    vf8_buf_destroy(buf);
}

void test_leb_misc()
{
    test_leb(32);
    test_leb(4096);
    test_leb(524288);
    test_leb(67108864);
    test_leb(8589934592);
    test_leb(1099511627776);
    test_leb(140737488355328);
    test_leb(18014398509481984);
}

void test_vlu_misc()
{
    test_vlu(32);
    test_vlu(4096);
    test_vlu(524288);
    test_vlu(67108864);
    test_vlu(8589934592);
    test_vlu(1099511627776);
    test_vlu(140737488355328);
    test_vlu(18014398509481984);
}

void test_vluc_byval_misc()
{
    test_vlu_byval(32);
    test_vlu_byval(4096);
    test_vlu_byval(524288);
    test_vlu_byval(67108864);
    test_vlu_byval(8589934592);
    test_vlu_byval(1099511627776);
    test_vlu_byval(140737488355328);
    test_vlu_byval(18014398509481984);
}

int main(int argc, const char **argv)
{
    test_ber_pi();
    test_vf8_loop();
    test_vf8_adhoc();
    test_leb_misc();
    test_vlu_misc();
    test_vluc_byval_misc();
}
