// RUN: %clang_cc1 -analyze -analyzer-checker=alpha.unix.ShiftingChecker -verify %s

void test()
{
    int x = 123;
    int a  = -1;
    x << a; // expected-warning {{Shifting by a negative value}}
}

// expected-warning@7 {{expression result unused}}
