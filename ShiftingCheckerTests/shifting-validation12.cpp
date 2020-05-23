// RUN: %clang_analyze_cc1 -Wno-unused -analyzer-checker=alpha.unix.ShiftingChecker -verify %s

#include "Inputs/system-header-simulator.h"

void test()
{
    int x = 42;
    int a;

    fscanf(stdin, "%d", &a);
    x << a; // expected-warning {{Shifting with a tainted value}}
}

