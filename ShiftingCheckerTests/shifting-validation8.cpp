// RUN: %clang_analyze_cc1 -Wno-unused -analyzer-checker=alpha.unix.ShiftingChecker -verify %s

void test(unsigned int si_a, unsigned int si_b) {
    signed long result;
    if (si_a > (9223372036854775807 >> si_b)) { // expected-warning {{Shifting with an undefined value}}
        /* Handle error */
    } else {
        result = si_a << si_b; // expected-warning {{Shifting with an undefined value}}
    }
}
