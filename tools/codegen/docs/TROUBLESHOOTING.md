# Troubleshooting Guide

Common issues and solutions for the Alloy code generator.

## Table of Contents

- [Installation Issues](#installation-issues)
- [SVD Sync Issues](#svd-sync-issues)
- [Parser Issues](#parser-issues)
- [Generator Issues](#generator-issues)
- [CMake Integration Issues](#cmake-integration-issues)
- [Generated Code Issues](#generated-code-issues)
- [Performance Issues](#performance-issues)

---

## Installation Issues

### Python Not Found

**Error:**
```
python3: command not found
```

**Solution:**
```bash
# macOS
brew install python3

# Ubuntu/Debian
sudo apt-get install python3

# Windows
# Download from python.org
```

### Jinja2 Not Installed

**Error:**
```
ImportError: No module named 'jinja2'
ERROR: jinja2 not installed. Install with: pip install jinja2
```

**Solution:**
```bash
pip3 install jinja2

# Or install all requirements
pip3 install -r tools/codegen/requirements.txt
```

### Permission Denied on Scripts

**Error:**
```
bash: ./run_tests.sh: Permission denied
```

**Solution:**
```bash
chmod +x tools/codegen/*.sh
chmod +x tools/codegen/*.py
```

---

## SVD Sync Issues

### Git Submodule Not Initialized

**Error:**
```
fatal: not a git repository (or any of the parent directories): .git
```

**Solution:**
```bash
cd /path/to/alloy  # Go to repository root
git submodule update --init --recursive
```

### Can't Clone CMSIS-SVD Repository

**Error:**
```
fatal: unable to access 'https://github.com/cmsis-svd/cmsis-svd-data.git/':
Could not resolve host: github.com
```

**Solutions:**
1. Check internet connection
2. Check if behind proxy:
   ```bash
   git config --global http.proxy http://proxy.company.com:8080
   ```
3. Use SSH instead of HTTPS:
   ```bash
   # Edit .gitmodules
   url = git@github.com:cmsis-svd/cmsis-svd-data.git
   ```

### Submodule Already Exists

**Error:**
```
A git directory for 'tools/codegen/upstream/cmsis-svd-data' is found locally
```

**Solution:**
```bash
# Remove existing submodule
git submodule deinit -f tools/codegen/upstream/cmsis-svd-data
git rm -f tools/codegen/upstream/cmsis-svd-data
rm -rf .git/modules/tools/codegen/upstream/cmsis-svd-data

# Re-initialize
cd tools/codegen
python3 sync_svd.py --init
```

### Symlink Creation Fails

**Error:**
```
✗ Failed to create symlink for STMicro: ... is not in the subpath
```

**Cause:** This is a known limitation of Python's `Path.relative_to()` on some systems.

**Workaround:** Ignore this error - it doesn't affect functionality. SVD files are still accessible in `upstream/cmsis-svd-data/data/`.

---

## Parser Issues

### SVD File Not Found

**Error:**
```
✗ SVD file not found: /path/to/file.svd
```

**Solutions:**
1. Check file path:
   ```bash
   ls tools/codegen/upstream/cmsis-svd-data/data/STMicro/STM32F103xx.svd
   ```

2. Sync SVD repository first:
   ```bash
   python3 sync_svd.py --init
   ```

3. Download SVD manually from vendor website

### XML Parse Error

**Error:**
```
✗ Failed to parse SVD XML: syntax error: line 123: ...
```

**Solutions:**
1. Validate SVD file:
   ```bash
   xmllint --noout file.svd
   ```

2. Check for common issues:
   - Missing closing tags
   - Invalid characters
   - Incorrect XML declaration

3. Try another SVD version from vendor

### Missing Flash/RAM Info

**Warning:**
```
⚠ Flash info not found in SVD, using defaults
⚠ RAM info not found in SVD, using defaults
```

**Solution:** This is normal - many SVD files don't include memory info. Edit the generated JSON:

```json
{
  "flash": {
    "size_kb": 128,  // Update from datasheet
    "base_address": "0x08000000"
  },
  "ram": {
    "size_kb": 32,   // Update from datasheet
    "base_address": "0x20000000"
  }
}
```

### Duplicate Interrupt Vectors

**Error:**
```
✗ Found 5 error(s):
  • MCU.interrupts.vectors[30]: Duplicate vector number 34
```

**Cause:** Some SVD files have duplicate interrupts (shared peripherals).

**Solution:** The parser now automatically handles this (keeps first occurrence). If you still see errors, manually edit the JSON to remove duplicates.

### Peripheral Classification Issues

**Warning:**
```
⚠ Unknown peripheral type: MY_PERIPH
```

**Solution:** This is informational only. The peripheral is still parsed. To improve classification, edit `svd_parser.py`:

```python
def _classify_peripheral(self, name: str) -> str:
    # Add your pattern
    if "MY_PERIPH" in name:
        return "MY_TYPE"
    ...
```

---

## Generator Issues

### Database Not Found

**Error:**
```
ERROR: Database not found: /path/to/database.json
```

**Solutions:**
1. Check path:
   ```bash
   ls tools/codegen/database/families/stm32f1xx.json
   ```

2. Parse SVD first:
   ```bash
   python3 svd_parser.py --input file.svd --output database/families/family.json
   ```

### MCU Not in Database

**Error:**
```
✗ MCU 'CUSTOM_MCU' not found in database
✗ Available MCUs: STM32F103C8, STM32F103CB
```

**Solutions:**
1. Check MCU name spelling (case-sensitive)
2. Check what's in the database:
   ```bash
   cat database/families/stm32f1xx.json | jq '.mcus | keys'
   ```
3. Parse SVD with `--merge` to add MCU:
   ```bash
   python3 svd_parser.py \
       --input new_mcu.svd \
       --output database/families/family.json \
       --merge
   ```

### Invalid JSON in Database

**Error:**
```
✗ Invalid JSON in database: Expecting ',' delimiter: line 45 column 5
```

**Solutions:**
1. Validate JSON:
   ```bash
   python3 -m json.tool database/families/family.json
   ```

2. Use online JSON validator (jsonlint.com)

3. Common issues:
   - Missing commas
   - Trailing commas
   - Unquoted keys
   - Invalid escape sequences

### Template Not Found

**Error:**
```
✗ Template rendering failed (startup/cortex_m_startup.cpp.j2):
jinja2.exceptions.TemplateNotFound: startup/cortex_m_startup.cpp.j2
```

**Solutions:**
1. Check template exists:
   ```bash
   ls tools/codegen/templates/startup/cortex_m_startup.cpp.j2
   ```

2. Run from correct directory:
   ```bash
   cd tools/codegen
   python3 generator.py --mcu MCU --database db.json --output /tmp/out
   ```

### Template Rendering Error

**Error:**
```
✗ Template rendering failed: undefined variable 'mcu_name'
```

**Cause:** Template references variable not in context.

**Solution:** Check that database has required fields. Edit template or database to match.

---

## CMake Integration Issues

### codegen.cmake Not Found

**Error:**
```
CMake Error: include could not find requested file:
  codegen
```

**Solution:** Add cmake directory to module path:

```cmake
list(APPEND CMAKE_MODULE_PATH ${ALLOY_ROOT}/cmake)
include(codegen)
```

### Code Generation Disabled

**Warning:**
```
-- Code generation: DISABLED (Python3 not found)
```

**Solution:**
```bash
# Install Python 3
brew install python3  # macOS
sudo apt install python3  # Linux

# Reconfigure CMake
rm -rf build
cmake -B build
```

### Database File Not Found in CMake

**Error:**
```
alloy_generate_code: Database file not found: /path/to/db.json
  MCU: STM32F103C8
  Available databases in /path/to/database/families/:
    - Run 'ls /path/to/database/families/' to see available families
```

**Solutions:**
1. Specify FAMILY parameter:
   ```cmake
   alloy_generate_code(
       MCU STM32F103C8
       FAMILY stm32f1xx  # Auto-finds database
   )
   ```

2. Specify full DATABASE path:
   ```cmake
   alloy_generate_code(
       MCU STM32F103C8
       DATABASE ${CMAKE_SOURCE_DIR}/tools/codegen/database/families/stm32f1xx.json
   )
   ```

### Stale Generated Code

**Problem:** Database changes don't appear in generated code.

**Cause:** Marker file `.generated` timestamp is newer than database.

**Solution:**
```bash
# Remove marker to force regeneration
rm build/generated/STM32F103C8/.generated

# Or remove entire generated directory
rm -rf build/generated

# Reconfigure
cmake -B build
```

### CMake Can't Execute Generator

**Error:**
```
execute_process given unknown argument "-mcu"
```

**Cause:** Wrong CMake syntax or old CMake version.

**Solutions:**
1. Update CMake:
   ```bash
   cmake --version  # Should be 3.25+
   ```

2. Check command syntax in `cmake/codegen.cmake`

---

## Generated Code Issues

### Compilation Errors

**Error:**
```
startup.cpp:67:45: error: aliases are not supported on darwin
```

**Cause:** Trying to compile ARM code with host compiler, or alias not supported on platform.

**Solutions:**
1. Use correct toolchain:
   ```cmake
   set(CMAKE_TOOLCHAIN_FILE cmake/toolchains/arm-none-eabi.cmake)
   ```

2. For Darwin/macOS development, ignore this - it's only an issue when compiling for host. The code is correct for ARM targets.

**Error:**
```
startup.cpp:7:10: fatal error: cstdint: No such file or directory
```

**Cause:** Missing C++ standard library for embedded target.

**Solution:** Add `-nostdlib` and provide `cstdint`:
```cmake
target_compile_options(firmware PRIVATE -nostdlib -ffreestanding)
target_include_directories(firmware PRIVATE ${ARM_TOOLCHAIN_INCLUDE})
```

### Linker Errors

**Error:**
```
undefined reference to `_sidata'
undefined reference to `_sdata'
```

**Cause:** Missing linker script defining these symbols.

**Solution:** Add linker script:
```cmake
target_link_options(firmware PRIVATE
    -T${CMAKE_SOURCE_DIR}/linker/STM32F103C8.ld
)
```

**Linker script should define:**
```ld
SECTIONS
{
    .text : {
        _sidata = LOADADDR(.data);
        ...
    }

    .data : {
        _sdata = .;
        ...
        _edata = .;
    }

    .bss : {
        _sbss = .;
        ...
        _ebss = .;
    }
}
```

### Runtime Crashes

**Problem:** MCU immediately hard faults after reset.

**Possible causes:**

1. **Stack pointer not set:** Check vector table has `Initial_SP`
2. **Incorrect memory addresses:** Verify flash/RAM addresses in database
3. **Clock not configured:** Implement `SystemInit()`
4. **Peripheral not enabled:** Enable clocks before accessing peripherals

**Debug steps:**
```cpp
// 1. Add breakpoint in Reset_Handler
extern "C" void Reset_Handler() {
    __asm__ volatile("bkpt");  // Breakpoint here
    // Step through initialization
    ...
}

// 2. Check stack pointer
uint32_t sp;
__asm__ volatile("mov %0, sp" : "=r"(sp));
printf("SP: 0x%08X\n", sp);

// 3. Enable fault handlers
extern "C" void HardFault_Handler() {
    while (1) {
        __asm__ volatile("bkpt");  // Debug hard fault
    }
}
```

### Missing Interrupt Handlers

**Problem:** Some interrupts don't have handlers.

**Solution:** The generated code provides weak default handlers for all interrupts. Override the ones you need:

```cpp
extern "C" void USART1_IRQHandler() {
    // Your handler
}
```

---

## Performance Issues

### Slow Code Generation

**Problem:** CMake configuration takes too long.

**Causes:**
1. Large SVD file (many peripherals/registers)
2. Python slow to start
3. Jinja2 template complexity

**Solutions:**
1. Use caching (default behavior):
   ```cmake
   # Code only regenerates when needed
   alloy_generate_code(MCU STM32F103C8)
   ```

2. Reduce database size (remove unused peripherals):
   ```json
   {
     "peripherals": {
       "GPIO": { ... },    // Keep
       "USART": { ... },   // Keep
       "FSMC": { ... }     // Remove if not using
     }
   }
   ```

3. Pre-generate and commit code (not recommended):
   ```bash
   python3 generator.py --mcu MCU --database db.json --output src/generated
   git add src/generated
   ```

### Large Generated Files

**Problem:** `startup.cpp` is very large (>10KB).

**Cause:** Many interrupt vectors (60+).

**Impact:** Minimal - modern compilers optimize unused functions away.

**If concerned:** Use linker garbage collection:
```cmake
target_compile_options(firmware PRIVATE -ffunction-sections -fdata-sections)
target_link_options(firmware PRIVATE -Wl,--gc-sections)
```

---

## Getting Help

If you're still stuck:

1. **Check existing issues:** https://github.com/anthropics/alloy/issues
2. **Run with verbose mode:**
   ```bash
   python3 svd_parser.py --input file.svd --output out.json --verbose
   python3 generator.py --mcu MCU --database db.json --output /tmp/out --verbose
   ```
3. **Validate your database:**
   ```bash
   python3 validate_database.py database/families/family.json
   ```
4. **Run tests:**
   ```bash
   cd tools/codegen
   ./run_tests.sh
   ```
5. **Create an issue** with:
   - Error message
   - Command you ran
   - Database file (if relevant)
   - CMakeLists.txt (if CMake issue)
   - Output of `--verbose` mode

## Common Workflows

### Debugging a Parser Issue

```bash
# 1. Run parser with verbose
python3 svd_parser.py --input file.svd --output /tmp/test.json --verbose

# 2. Check what was parsed
cat /tmp/test.json | jq '.mcus | keys'
cat /tmp/test.json | jq '.mcus.MCU_NAME.peripherals | keys'

# 3. Validate
python3 validate_database.py /tmp/test.json

# 4. Test generation
python3 generator.py --mcu MCU_NAME --database /tmp/test.json --output /tmp/gen -v
```

### Debugging a CMake Issue

```bash
# 1. Clean build
rm -rf build

# 2. Configure with verbose
cmake -B build --debug-output 2>&1 | tee cmake_log.txt

# 3. Check generator was called
grep "Generating code" cmake_log.txt

# 4. Check generated files
ls -la build/generated/*/

# 5. Test generator manually
python3 tools/codegen/generator.py \
    --mcu MCU_NAME \
    --database tools/codegen/database/families/family.json \
    --output /tmp/test \
    --verbose
```

### Fixing a Corrupted Database

```bash
# 1. Validate to see errors
python3 validate_database.py database/families/family.json

# 2. Format JSON nicely
python3 -m json.tool database/families/family.json > /tmp/formatted.json
mv /tmp/formatted.json database/families/family.json

# 3. Check with jq
cat database/families/family.json | jq .

# 4. Re-parse from SVD if too broken
python3 svd_parser.py --input original.svd --output database/families/family.json
```

