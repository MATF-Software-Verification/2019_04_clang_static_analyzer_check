// RUN: %clang_cc1 -analyze -analyzer-checker=unix,alpha -fsyntax-only %s

void test()
{
    int x = 123;
    int y = 10;
    int z = -10;
    int a = 1111111;

    int r;

    r = x << y;
    r = x << z; // expected-warning {{shifting should be done with a positive value}}
    r = x >> a; // expected-warning {{shifting should be less than number of bits of the integer type}}

    r <<= z; // expected-warning {{shifting should be done with a positive value}}
    r >>= a; // expected-warning {{shifting should be less than number of bits of the integer type}}
}
