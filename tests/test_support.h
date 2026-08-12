#pragma once

// Minimal assertion helpers shared by the test binaries. No framework: these
// tests are linked into tiny standalone executables that ctest runs, and a
// pass/fail line per check is all the output a CI log needs.

#include <cmath>
#include <cstdio>

namespace ace_test {

inline int g_failures = 0;

// Returns what it checked, so a test that cannot meaningfully continue past a
// failed precondition can bail out instead of cascading. Most call sites ignore
// it.
inline bool Check(bool cond, const char* what) {
    if (cond) {
        std::printf("  ok:   %s\n", what);
        return true;
    }
    std::printf("  FAIL: %s\n", what);
    ++g_failures;
    return false;
}

// Tolerance-based compare for values that go through trig. 1e-5 is far tighter
// than any real regression in the camera math and far looser than the float
// rounding of a sin/cos round trip.
constexpr float kEpsilon = 1e-5f;

inline void CheckClose(float actual, float expected, const char* what) {
    const bool ok = std::fabs(actual - expected) <= kEpsilon;
    if (ok) {
        std::printf("  ok:   %s\n", what);
        return;
    }
    std::printf("  FAIL: %s (expected %.6f, got %.6f)\n", what, expected, actual);
    ++g_failures;
}

inline int Summary(const char* suite) {
    if (g_failures == 0) {
        std::printf("\n%s: all tests passed\n", suite);
        return 0;
    }
    std::printf("\n%s: %d test(s) FAILED\n", suite, g_failures);
    return 1;
}

}  // namespace ace_test
