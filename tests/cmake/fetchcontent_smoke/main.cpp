// Smoke target for the FetchContent / add_subdirectory consumption path.
//
// We don't actually exercise the alloy runtime here — the configure-time
// asserts in this test's CMakeLists.txt prove the helper + target are
// available.  Linking against Alloy::hal is verification enough.

int main() {
    return 0;
}
