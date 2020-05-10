// RUN: %clang_analyze_cc1 -Wno-unused -analyzer-checker=alpha.unix.ShiftingChecker -verify %s

void fn(char &a) { a++; }
void test()
{
    char x = 123;
    char a  = 7;
    int c;
    fn(a);
    x << a; // expected-warning {{Invalid shift operation. Type 'char' can't be shifted by more than '6' bits}}

}
