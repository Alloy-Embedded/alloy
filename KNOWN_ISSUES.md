# Known Issues

This document tracks known issues and limitations in the Alloy project.

## Sanitizer Builds Disabled (Temporary)

**Status**: Temporarily disabled in CI
**Affected**: AddressSanitizer and UndefinedBehaviorSanitizer builds
**Root Cause**: Toolchain incompatibility

### Issue Description

The sanitizer builds are currently disabled due to a known incompatibility between:
- GCC 14 libstdc++ implementation
- Clang 14 compiler
- Catch2 test framework
- C++20 features (`<chrono>` and `<format>` headers)

### Error Symptoms

When building Catch2 with Clang 14 and GCC 14's libstdc++, compilation fails with errors like:

```
error: call to consteval function 'std::chrono::hh_mm_ss::_S_fractional_width'
       is not a constant expression
error: no matching function for call to '__begin'
```

### Why This Happens

GCC 14's libstdc++ has a known bug in its C++20 `<format>` library implementation that causes issues when compiled with Clang 14. Catch2's use of `<chrono>` triggers these bugs when building with sanitizers enabled.

### This is NOT a Alloy Code Issue

This is purely a toolchain incompatibility. The Alloy codebase is correct and builds successfully without sanitizers.

### Solutions

We have several options to re-enable sanitizer builds:

1. **Upgrade to Clang 18+** (Recommended)
   - Clang 18 has better C++20 support and compatibility with GCC 14 libstdc++
   - Update `.github/workflows/ci.yml` to use `clang-18` instead of `clang-14`

2. **Use GCC 13 Instead of GCC 14**
   - GCC 13's libstdc++ doesn't have this bug
   - Requires downgrading system libstdc++ or using a container

3. **Disable Catch2 When Building with Sanitizers**
   - Build sanitizer versions without unit tests
   - Less useful as it reduces test coverage

4. **Wait for GCC 14 libstdc++ Fix**
   - Track upstream LLVM issue: https://github.com/llvm/llvm-project/issues/63773
   - May take several months

### Current Workaround

Sanitizer builds are temporarily disabled in CI via:

```yaml
sanitizers:
  name: Sanitizers (AddressSanitizer + UBSan)
  runs-on: ubuntu-latest
  if: false  # Temporarily disabled
```

### When Will This Be Re-enabled?

We will re-enable sanitizer builds when:
- Ubuntu GitHub runners upgrade to Clang 18+, OR
- We manually install Clang 18 in the CI workflow, OR
- GCC 14 libstdc++ bug is fixed upstream

### Testing Sanitizers Locally

You can still test with sanitizers locally if you have Clang 18+ or GCC 13:

```bash
# With Clang 18+
cmake -S . -B build-sanitizers \
  -DCMAKE_BUILD_TYPE=Debug \
  -DALLOY_BOARD=host \
  -DALLOY_BUILD_TESTS=ON \
  -DCMAKE_C_COMPILER=clang-18 \
  -DCMAKE_CXX_COMPILER=clang++-18 \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"

cmake --build build-sanitizers
cd build-sanitizers && ctest
```

### References

- LLVM Issue: https://github.com/llvm/llvm-project/issues/63773
- GCC Bugzilla: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=110167
- Related Catch2 Discussion: https://github.com/catchorg/Catch2/issues/2421

---

**Last Updated**: 2025-11-08
**Tracking Issue**: #TBD (create issue if needed)
