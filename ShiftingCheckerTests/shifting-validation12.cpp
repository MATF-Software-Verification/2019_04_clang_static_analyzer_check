// RUN: %clang --analyze -Wno-unused -Xanalyzer -analyzer-checker=alpha.unix.ShiftingChecker,alpha.security.taint,debug.TaintTest -Xclang -verify %s

#include <stdio.h>

void test()
{
    int x = 42;
    int a;

    fscanf(stdin, "%d", &a);
    x << a; // expected-warning {{Shifting with a tainted value}}
}

// expected-warning@11 2 {{tainted}}
