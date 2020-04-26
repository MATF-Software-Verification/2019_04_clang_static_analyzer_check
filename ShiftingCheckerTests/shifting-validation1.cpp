// RUN: %clang_cc1 -analyze -analyzer-checker=alpha.unix.ShiftingChecker -verify %s

void test()
{
    int x = 123;
    int a  = 32;
    x << a; // expected-warning {{Invalid shift operation. Type 'int' can't be shifted by more than '31' bits}}
}

// expected-warning@7 {{expression result unused}}
