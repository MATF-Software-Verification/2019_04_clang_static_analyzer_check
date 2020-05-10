// RUN: %clang_analyze_cc1 -Wno-unused -analyzer-checker=alpha.unix.ShiftingChecker -verify %s
// expected-no-diagnostics

void test(unsigned long si_a, unsigned long si_b) {
    signed long result;
    if ((si_a < 0) || (si_b < 0) ||
        (si_b >= 63) || 
        (si_a > (9223372036854775807 >> si_b))) {
        /* Handle error */
    } else {
        result = si_a >> si_b;
    }
}
