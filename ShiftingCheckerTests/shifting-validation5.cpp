// RUN: %clang_analyze_cc1 -Wno-unused -analyzer-checker=alpha.unix.ShiftingChecker -verify %s
// expected-no-diagnostics

void test()
{
    unsigned x = 123;
    unsigned a  = 31;
    x << a;
}

