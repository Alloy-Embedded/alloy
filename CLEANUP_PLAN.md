# Cleanup Plan - Vendor Directory Reorganization

**Date**: 2025-11-11
**Status**: ✅ COMPLETE
**See**: CLEANUP_COMPLETE.md for execution report

---

## 🎯 Problems Identified

### Problem 1: Wrong Vendor Path ❌
**Current**: `src/hal/vendors/arm/same70/`
**Correct**: `src/hal/vendors/atmel/same70/`

**Files affected**:
- `systick_hardware_policy.hpp` - Generated Phase 14
- `nvic_hardware_policy.hpp` - Generated Phase 14
- `startup_config.hpp` - Created Phase 14.1
- `startup_same70.cpp` - Created Phase 14.1

**Issue**: SAME70 is an Atmel/Microchip product, not generic ARM. These files belong in `vendors/atmel/same70/`.

---

### Problem 2: Old Generated Files per MCU ❌
**Location**: `src/hal/vendors/atmel/same70/atsame70*/`

**Files to DELETE** (not used anymore):
1. **startup.cpp** - Old startup (replaced by Phase 14 modern startup)
2. **pins.hpp** - Old pin definitions (replaced by signal routing system)
3. **pin_functions.hpp** - Old pin functions (replaced by signal routing)

**Files to KEEP**:
- `register_map.hpp` - Still used for register definitions
- `peripherals.hpp` - Still used for peripheral addresses
- `enums.hpp` - Still used for peripheral enums

**Reason**: We now use:
- Modern startup from Phase 14 (constexpr vector table)
- Signal routing system from Phase 2-3
- Pin/peripheral connection from multi-level APIs

---

### Problem 3: Signal Files Location ❌
**Current**: `src/hal/vendors/atmel/same70/same70_signals.hpp` (family level)
**Correct**: `src/hal/vendors/atmel/same70/atsame70q21b/signals.hpp` (MCU level)

**Issue**: Different MCUs have different pin counts:
- ATSAME70J** - 64 pins
- ATSAME70N** - 100 pins
- ATSAME70Q** - 144 pins

Each MCU needs its own `signals.hpp` because available signals change with pin count.

---

### Problem 4: Old Generators ❓
Need to check if these still exist and should be removed:
- Old startup generator (replaced by `startup_generator.py`)
- Old pin generator
- Old pin_functions generator

---

## 📋 Cleanup Actions

### Action 1: Move ARM → Atmel Files ✅

**Move these files**:
```bash
# From vendors/arm/same70/ to vendors/atmel/same70/
mv src/hal/vendors/arm/same70/systick_hardware_policy.hpp \
   src/hal/vendors/atmel/same70/

mv src/hal/vendors/arm/same70/nvic_hardware_policy.hpp \
   src/hal/vendors/atmel/same70/

mv src/hal/vendors/arm/same70/startup_config.hpp \
   src/hal/vendors/atmel/same70/

mv src/hal/vendors/arm/same70/startup_same70.cpp \
   src/hal/vendors/atmel/same70/
```

**Update includes** in:
- `src/hal/platform/same70/systick.hpp`
- `src/hal/platform/same70/nvic.hpp`
- `boards/same70_xplained/board.cpp`
- Any other files referencing these

**Delete empty directory**:
```bash
rmdir src/hal/vendors/arm/same70/
```

---

### Action 2: Delete Old Generated Files ❌

**For each MCU directory** (`atsame70j*`, `atsame70n*`, `atsame70q*`):

```bash
# Delete old startup files
find src/hal/vendors/atmel/same70/atsame70*/  -name "startup.cpp" -delete

# Delete old pin files
find src/hal/vendors/atmel/same70/atsame70*/ -name "pins.hpp" -delete
find src/hal/vendors/atmel/same70/atsame70*/ -name "pin_functions.hpp" -delete
find src/hal/vendors/atmel/same70/atsame70*/ -name "pin_functions_template.json" -delete
```

**Files remaining per MCU**:
- `register_map.hpp` ✅
- `peripherals.hpp` ✅
- `enums.hpp` ✅
- (New) `signals.hpp` ✅

---

### Action 3: Move Signals to MCU Level 🔄

**Current**:
```
src/hal/vendors/atmel/same70/
  └── same70_signals.hpp  ❌ (family level)
```

**Should be**:
```
src/hal/vendors/atmel/same70/
  ├── atsame70j19b/
  │   └── signals.hpp  ✅ (64-pin signals)
  ├── atsame70n21b/
  │   └── signals.hpp  ✅ (100-pin signals)
  └── atsame70q21b/
      └── signals.hpp  ✅ (144-pin signals)
```

**Steps**:
1. Check which MCU signals are currently in `same70_signals.hpp`
2. Move to appropriate MCU directory
3. Generate signals for other MCUs (or mark as TODO)
4. Delete `same70_signals.hpp`

---

### Action 4: Find and Remove Old Generators 🔍

**Check for**:
```bash
find tools/codegen -name "*startup*" -not -name "startup_generator.py" -not -name "startup.cpp.j2"
find tools/codegen -name "*pins*"
find tools/codegen -name "*pin_functions*"
```

**Archive or delete**:
- Old generators that are no longer used
- Old templates replaced by new system

---

## 📊 Impact Analysis

### Files Affected Summary

| Category | Action | Count | Impact |
|----------|--------|-------|--------|
| Hardware policies | Move | 4 files | Update includes |
| Old startup.cpp | Delete | ~10 files | None (not used) |
| Old pins.hpp | Delete | ~10 files | None (replaced) |
| Old pin_functions.hpp | Delete | ~10 files | None (replaced) |
| Signal files | Move/Split | 1 → N | MCU-specific |
| Old generators | Archive | TBD | None |

**Total deletions**: ~30 files
**Include updates**: ~5 files

---

## ✅ Validation Steps

After cleanup:

1. **Build Check**:
   ```bash
   make clean
   make all
   ```

2. **Include Check**:
   ```bash
   grep -r "vendors/arm/same70" src/ boards/ examples/
   # Should return 0 results
   ```

3. **Startup Check**:
   ```bash
   # Verify modern startup is used
   grep -r "startup_same70.cpp" boards/
   ```

4. **Signal Check**:
   ```bash
   # Verify signals are at MCU level
   find src/hal/vendors/atmel/same70/atsame70*/ -name "signals.hpp"
   ```

---

## 🚨 Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Broken includes | Medium | High | Update all includes first |
| Build failures | Low | Medium | Test build after each step |
| Missing signals | Medium | Medium | Generate per MCU |
| Lost old code | Low | Low | Old code not used |

**Overall Risk**: LOW (old files are not used in current system)

---

## 📝 Order of Execution

**Phase 1: Backup** (Safety)
```bash
git checkout -b cleanup/vendor-reorganization
git add -A
git commit -m "Backup before vendor cleanup"
```

**Phase 2: Move ARM → Atmel**
1. Move 4 hardware policy files
2. Update includes
3. Test build
4. Commit

**Phase 3: Delete Old Generated Files**
1. Delete startup.cpp files
2. Delete pins.hpp files
3. Delete pin_functions.hpp files
4. Test build
5. Commit

**Phase 4: Reorganize Signals**
1. Identify current MCU in same70_signals.hpp
2. Move to MCU directory
3. Update includes
4. Test build
5. Commit

**Phase 5: Clean Generators**
1. Find old generators
2. Archive or delete
3. Update documentation
4. Commit

---

## 🎯 Expected Result

**Before**:
```
src/hal/vendors/
├── arm/same70/ ❌
│   ├── systick_hardware_policy.hpp
│   ├── nvic_hardware_policy.hpp
│   ├── startup_config.hpp
│   └── startup_same70.cpp
└── atmel/same70/
    ├── same70_signals.hpp ❌
    ├── atsame70q21b/
    │   ├── startup.cpp ❌
    │   ├── pins.hpp ❌
    │   ├── pin_functions.hpp ❌
    │   ├── register_map.hpp ✅
    │   └── peripherals.hpp ✅
    └── (other MCUs...)
```

**After**:
```
src/hal/vendors/
├── arm/cortex_m7/ ✅
│   ├── vector_table.hpp
│   ├── startup_impl.hpp
│   └── init_hooks.hpp
└── atmel/same70/ ✅
    ├── systick_hardware_policy.hpp ✅
    ├── nvic_hardware_policy.hpp ✅
    ├── startup_config.hpp ✅
    ├── startup_same70.cpp ✅
    ├── atsame70q21b/
    │   ├── register_map.hpp ✅
    │   ├── peripherals.hpp ✅
    │   ├── enums.hpp ✅
    │   └── signals.hpp ✅ (MCU-specific)
    └── (other MCUs...)
```

---

## 📚 Documentation Updates

After cleanup, update:
1. `PHASE_14_COMPLETE.md` - Reflect new file locations
2. `ARCHITECTURE.md` - Update vendor path examples
3. `README.md` - Update directory structure
4. `MIGRATION_GUIDE.md` - Note old files removed

---

**Ready for Execution?** YES / NO

**Estimated Time**: 1-2 hours
**Complexity**: MEDIUM
**Risk**: LOW
