// RUN: %clang_analyze_cc1 -Wno-unused -analyzer-checker=alpha.unix.ShiftingChecker -verify %s
// expected-no-diagnostics

void test(unsigned int ui_a, unsigned int ui_b) {
    unsigned int uresult = 0;
    if (ui_b >= 32) {
        /* Handle error */
    } else {
        uresult = ui_a >> ui_b;
    }
}
