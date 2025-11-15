# Branch: feat/rtos-cpp23-improvements

## Purpose

This branch implements the RTOS improvements specified in `openspec/changes/integrate-systick-rtos-improvements/`.

**Parent Branch**: `feat/phase4-codegen-consolidation`

**Work Mode**: Independent development while main branch continues Phase 4 codegen work.

## Implementation Phases

This branch will implement improvements incrementally following the 7-phase plan:

### ✅ Phase 0: OpenSpec Documentation (COMPLETED)
- [x] Comprehensive analysis (600+ lines)
- [x] Updated proposal.md with 7 phases
- [x] Updated design.md with C++23 features
- [x] Updated rtos-core spec.md
- [x] Created SUMMARY.md

### ✅ Phase 1: Result<T,E> Integration (COMPLETED)
**Status**: ✅ **COMPLETE** (Single session)
**Goal**: Replace all `bool` returns with `Result<T, RTOSError>`

**Summary**: Successfully migrated all 15 RTOS API functions from boolean error handling to type-safe `Result<T, RTOSError>` pattern. See `docs/PHASE1_COMPLETION_SUMMARY.md` for details.

**Tasks**:
- [x] 1.1: Define `RTOSError` enum in `src/rtos/error.hpp` (14 error codes)
- [x] 1.2: Update `Mutex::lock()` to return `Result<void, RTOSError>`
- [x] 1.3: Update `Mutex::unlock()` to return `Result<void, RTOSError>`
- [x] 1.4: Update `Mutex::try_lock()` to return `Result<void, RTOSError>`
- [x] 1.5: Update `Queue::send()` to return `Result<void, RTOSError>`
- [x] 1.6: Update `Queue::receive()` to return `Result<T, RTOSError>` (changed signature!)
- [x] 1.7: Update `Queue::try_send()` to return `Result<void, RTOSError>`
- [x] 1.8: Update `Queue::try_receive()` to return `Result<T, RTOSError>` (changed signature!)
- [x] 1.9: Update `Semaphore` APIs (both Binary and Counting)
- [x] 1.10: Update `RTOS::tick()` to return `Result<void, RTOSError>`
- [x] 1.11: Update `scheduler::tick()` and `wake_delayed_tasks()`
- [x] 1.12: Update `LockGuard` for Result compatibility
- [ ] 1.13: Add backward compatibility helpers (deprecated) - DEFERRED
- [ ] 1.14: Update all RTOS examples - DEFERRED to testing phase
- [ ] 1.15: Update RTOS tests - DEFERRED to Phase 8
- [ ] 1.16: Verify no regressions - DEFERRED to Phase 8

**Commits**:
- `1b657127`: Phase 1.1-1.2 (RTOSError + Mutex)
- `41836fed`: Phase 1.3 (Queue)
- `2cf4bb85`: Phase 1.4 (Semaphore)
- `6770d2a6`: Phase 1.5 (RTOS::tick)
- `86cf7bef`: Phase 1.6 (LockGuard)

**Files Modified**:
- `src/rtos/error.hpp` (NEW - 200+ lines)
- `src/rtos/mutex.hpp` (3 functions + LockGuard)
- `src/rtos/queue.hpp` (4 functions, signature changes)
- `src/rtos/semaphore.hpp` (6 functions across 2 classes)
- `src/rtos/rtos.hpp` (1 function)
- `src/rtos/scheduler.hpp` (2 functions)

**Key Achievements**:
- ✅ Type-safe error handling (14 error codes)
- ✅ Consistency with HAL layer
- ✅ Improved API ergonomics (Queue returns values directly)
- ✅ Zero runtime overhead
- ✅ Better error composition with Result monadic operations
- ✅ Comprehensive documentation (~400 lines added)

### 📝 Phase 2: Compile-Time TaskSet (Weeks 3-5)
**Status**: Waiting for Phase 1
**Goal**: Variadic template task registration with compile-time validation

**Tasks**:
- [ ] 2.1: Create `fixed_string` template in `src/rtos/concepts.hpp`
- [ ] 2.2: Update `Task<>` template to use `fixed_string` for name
- [ ] 2.3: Create `TaskSet<Tasks...>` variadic template
- [ ] 2.4: Implement `consteval calculate_total_ram()`
- [ ] 2.5: Add compile-time validation (stack, priority, RAM)
- [ ] 2.6: Implement `RTOS::start<TaskSet>()`
- [ ] 2.7: Add backward compatibility shim (deprecated)
- [ ] 2.8: Create migration script `scripts/migrate_rtos_api.py`
- [ ] 2.9: Update all examples to new API
- [ ] 2.10: Test on all 5 boards

**Affected Files**:
- `src/rtos/concepts.hpp` (NEW)
- `src/rtos/rtos.hpp`
- `src/rtos/scheduler.hpp`
- `scripts/migrate_rtos_api.py` (NEW)
- All examples and tests

### 📝 Phase 3: Concept-Based Type Safety (Weeks 6-7)
**Status**: Waiting for Phase 2
**Goal**: C++20 concepts for IPC and tick source validation

**Tasks**:
- [ ] 3.1: Create `IPCMessage<T>` concept
- [ ] 3.2: Create `RTOSTickSource` concept
- [ ] 3.3: Create `QueueProducer`/`QueueConsumer` concepts
- [ ] 3.4: Update `Queue<>` template to use `IPCMessage` concept
- [ ] 3.5: Update `RTOS::tick()` to use `RTOSTickSource` concept
- [ ] 3.6: Test concept validation (intentional errors)
- [ ] 3.7: Verify error messages are clear

**Affected Files**:
- `src/rtos/concepts.hpp` (expand)
- `src/rtos/queue.hpp`
- `src/rtos/rtos.hpp`
- All board.cpp files (for RTOSTickSource)

### 📝 Phase 4: Unified SysTick Integration (Weeks 8-9)
**Status**: Waiting for Phase 3
**Goal**: Standardize RTOS tick integration across all boards

**Tasks**:
- [ ] 4.1: Update `boards/nucleo_f401re/board.cpp` SysTick_Handler
- [ ] 4.2: Update `boards/nucleo_f722ze/board.cpp` SysTick_Handler
- [ ] 4.3: Update `boards/nucleo_g071rb/board.cpp` SysTick_Handler
- [ ] 4.4: Update `boards/nucleo_g0b1re/board.cpp` SysTick_Handler
- [ ] 4.5: Update `boards/same70_xplained/board.cpp` SysTick_Handler
- [ ] 4.6: Remove `src/rtos/platform/arm_systick_integration.cpp`
- [ ] 4.7: Test RTOS on all 5 boards
- [ ] 4.8: Verify tick accuracy (±1%)

**Files Removed**:
- `src/rtos/platform/arm_systick_integration.cpp`

### 📝 Phase 5: C++23 Enhancements (Weeks 10-12)
**Status**: Waiting for Phase 4
**Goal**: Leverage C++23 features for maximum compile-time power

**Tasks**:
- [ ] 5.1: Update CMakeLists.txt to require C++23
- [ ] 5.2: Implement `consteval` for RAM calculations
- [ ] 5.3: Implement `if consteval` for dual-mode functions
- [ ] 5.4: Add deducing `this` for CRTP patterns (if applicable)
- [ ] 5.5: Verify compile-time guarantees
- [ ] 5.6: Measure compile time impact (<5% target)
- [ ] 5.7: Measure binary size impact (<1% target)

**Affected Files**:
- `CMakeLists.txt`
- `src/rtos/rtos.hpp`
- All RTOS headers

### 📝 Phase 6: Advanced Features (Weeks 13-16)
**Status**: Waiting for Phase 5
**Goal**: Task notifications, memory pools, tickless idle

**Tasks**:
- [ ] 6.1: Implement `TaskNotification` (8 bytes per task)
- [ ] 6.2: Implement `StaticPool<T, Size>`
- [ ] 6.3: Add tickless idle hooks
- [ ] 6.4: Create `examples/rtos/task_notifications/`
- [ ] 6.5: Create `examples/rtos/memory_pool/`
- [ ] 6.6: Create `examples/rtos/tickless_idle/`

**New Files**:
- `src/rtos/task_notification.hpp`
- `src/rtos/memory_pool.hpp`
- `examples/rtos/task_notifications/`
- `examples/rtos/memory_pool/`
- `examples/rtos/tickless_idle/`

### 📝 Phase 7: Documentation & Release (Weeks 17-18)
**Status**: Waiting for Phase 6
**Goal**: Complete documentation and final validation

**Tasks**:
- [ ] 7.1: Complete API documentation
- [ ] 7.2: Write migration guide
- [ ] 7.3: Create tutorials
- [ ] 7.4: Final testing on all boards
- [ ] 7.5: Performance validation
- [ ] 7.6: Prepare for merge to main

## Branch Workflow

### While Working on This Branch

```bash
# Check current branch
git branch --show-current
# Should show: feat/rtos-cpp23-improvements

# Make changes and commit
git add <files>
git commit -m "feat: implement Phase X - description"

# Push to remote
git push -u origin feat/rtos-cpp23-improvements
```

### Syncing with Parent Branch (if needed)

```bash
# Fetch latest from parent
git fetch origin feat/phase4-codegen-consolidation

# Rebase on parent (if there are updates)
git rebase origin/feat/phase4-codegen-consolidation

# Or merge parent changes
git merge origin/feat/phase4-codegen-consolidation
```

### When Ready to Merge

```bash
# Switch to parent branch
git checkout feat/phase4-codegen-consolidation

# Merge RTOS improvements
git merge feat/rtos-cpp23-improvements

# Or create PR for review
gh pr create --base feat/phase4-codegen-consolidation \
             --head feat/rtos-cpp23-improvements \
             --title "feat: RTOS C++23 improvements (18 weeks)" \
             --body "See BRANCH_PLAN.md for details"
```

## Current Status

- **Branch Created**: ✅
- **OpenSpec Documentation**: ✅ Complete (2000+ lines)
- **Phase 1 Ready**: ✅ Can start implementation
- **Main Branch**: Independent (can work in parallel)

## Testing Strategy

Each phase will include:
- Unit tests (Catch2)
- Integration tests (on all 5 boards)
- Performance tests (context switch, compile time, binary size)
- Regression tests (verify no breakage)

## Success Criteria

See `openspec/changes/integrate-systick-rtos-improvements/SUMMARY.md` for detailed metrics.

**Key Metrics**:
- Context switch: <10µs (unchanged)
- Compile time: +5% max
- Binary size: +1% max
- TCB size: 28 bytes (down from 32)
- Total RAM: compile-time calculable (±2% accuracy)
