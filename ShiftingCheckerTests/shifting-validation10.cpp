// RUN: %clang_analyze_cc1 -Wno-unused -analyzer-checker=alpha.unix.ShiftingChecker -verify %s

void test(unsigned int ui_a, unsigned int ui_b) {
    unsigned int uresult = ui_a >> ui_b; // expected-warning {{Shifting with an undefined value}}
}
