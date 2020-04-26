// RUN: %clang_cc1 -analyze -analyzer-checker=alpha.unix.ShiftingChecker -verify %s

void fn(char &a) { a++; }
void test()
{
    char x = 123;
    char a  = 7;
    int c;
    fn(a);
    x << a; // expected-warning {{Invalid shift operation. Type 'char' can't be shifted by more than '7' bits}}

}

// expected-warning@10 {{expression result unused}}
