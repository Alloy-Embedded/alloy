# Implementation Tasks

## Phase 1: SysTick Integration (Weeks 1-2)

### 1.1 Standardize RTOS Tick Integration
- [x] 1.1.1 Define `RTOSTick` type alias in each board (✅ All 5 boards)
- [x] 1.1.2 Update all `SysTick_Handler()` to call `RTOS::tick()` (✅ Conditional with #ifdef ALLOY_RTOS_ENABLED)
- [x] 1.1.3 Add compile-time tick rate validation (must be 1ms for RTOS) (✅ static_assert added)
- [x] 1.1.4 Document RTOS tick pattern in all board.cpp files (✅ Updated comments)
- [ ] 1.1.5 Test RTOS tick accuracy on all 5 boards (±1%) (⚠️ Requires hardware testing)

### 1.2 RTOS Timing Validation
- [x] 1.2.1 Add `RTOS::get_tick_count()` implementation (✅ Already exists in scheduler.cpp)
- [x] 1.2.2 Add `RTOS::delay()` with millisecond accuracy (✅ Already exists in scheduler.cpp)
- [ ] 1.2.3 Add `RTOS::delay_until()` for absolute timing (📝 Deferred - future enhancement)
- [ ] 1.2.4 Validate timing with oscilloscope on 2+ boards (⚠️ Requires hardware)
- [ ] 1.2.5 Document timing accuracy guarantees (📝 Deferred - needs hardware validation first)

### 1.3 Tickless Idle Foundation
- [ ] 1.3.1 Design tickless idle interface (future)
- [ ] 1.3.2 Add placeholder for low-power integration
- [ ] 1.3.3 Document tickless requirements

## Phase 2: Compile-Time Task System (Weeks 3-4)

### 2.1 Variadic Task Registration
- [ ] 2.1.1 Create `TaskSet<Tasks...>` template
- [ ] 2.1.2 Implement `RTOS::start<TaskSet>()` entry point
- [ ] 2.1.3 Add compile-time task validation (stack size, priority)
- [ ] 2.1.4 Calculate total RAM at compile-time
- [ ] 2.1.5 Test with 1, 3, and 8 tasks

### 2.2 Task Configuration Validation
- [ ] 2.2.1 Add `static_assert` for minimum stack size (256 bytes)
- [ ] 2.2.2 Add `static_assert` for stack alignment (8 bytes)
- [ ] 2.2.3 Add `static_assert` for priority range (0-7)
- [ ] 2.2.4 Add optional priority uniqueness check
- [ ] 2.2.5 Add memory budget validation against available RAM

### 2.3 Backward Compatibility Shim
- [ ] 2.3.1 Maintain old Task constructor (with deprecation warning)
- [ ] 2.3.2 Create migration script (`migrate_rtos_api.py`)
- [ ] 2.3.3 Test migration script on example projects
- [ ] 2.3.4 Document migration process in guide
- [ ] 2.3.5 Add FAQ for common migration issues

## Phase 3: Type-Safe IPC (Weeks 5-6)

### 3.1 Message Type Concepts
- [ ] 3.1.1 Create `IPCMessage<T>` concept
- [ ] 3.1.2 Create `QueueProducer<Task, Queue>` concept
- [ ] 3.1.3 Create `QueueConsumer<Task, Queue>` concept
- [ ] 3.1.4 Update Queue template to use concepts
- [ ] 3.1.5 Test concept validation with intentional errors

### 3.2 Type-Safe Message Routing
- [ ] 3.2.1 Design message routing interface
- [ ] 3.2.2 Implement compile-time producer/consumer matching
- [ ] 3.2.3 Add type-safe send/receive wrappers
- [ ] 3.2.4 Create example demonstrating type safety
- [ ] 3.2.5 Document message routing patterns

### 3.3 Enhanced Synchronization
- [ ] 3.3.1 Implement `ScopedMultiLock<Mutexes...>` template
- [ ] 3.3.2 Add deadlock-free ordering by memory address
- [ ] 3.3.3 Test with 2, 3, and 4 mutexes
- [ ] 3.3.4 Add optional compile-time deadlock detection
- [ ] 3.3.5 Document multi-mutex patterns

## Phase 4: Error Handling (Weeks 7-8)

### 4.1 RTOSError Enum
- [ ] 4.1.1 Define `RTOSError` enum (Timeout, NotOwner, QueueFull, etc.)
- [ ] 4.1.2 Add error-to-string conversion
- [ ] 4.1.3 Document all error codes
- [ ] 4.1.4 Add error code unit tests

### 4.2 Result<T,E> Integration
- [ ] 4.2.1 Update `Mutex::lock()` to return `Result<void, RTOSError>`
- [ ] 4.2.2 Update `Queue::send()` to return `Result<void, RTOSError>`
- [ ] 4.2.3 Update `Queue::receive()` to return `Result<T, RTOSError>`
- [ ] 4.2.4 Update all RTOS APIs to use Result<T,E>
- [ ] 4.2.5 Test monadic operations (and_then, or_else)

### 4.3 Backward Compatibility Helpers
- [ ] 4.3.1 Create `.unwrap_or(false)` helper for bool compatibility
- [ ] 4.3.2 Add deprecation warnings to old APIs
- [ ] 4.3.3 Update migration script for Result<T,E> changes
- [ ] 4.3.4 Test compatibility layer
- [ ] 4.3.5 Document Result<T,E> migration

## Phase 5: Advanced Features (Weeks 9-10)

### 5.1 Task Notifications
- [ ] 5.1.1 Design `TaskNotification` API
- [ ] 5.1.2 Implement `notify()` and `wait_notification()`
- [ ] 5.1.3 Add timeout support for wait
- [ ] 5.1.4 Test notification from ISR
- [ ] 5.1.5 Compare footprint vs Queue (should be ~8 bytes vs 16+ bytes)

### 5.2 Static Memory Pools
- [ ] 5.2.1 Create `StaticPool<T, Size>` template
- [ ] 5.2.2 Implement `allocate()` and `deallocate()`
- [ ] 5.2.3 Add compile-time size validation
- [ ] 5.2.4 Add runtime pointer validation
- [ ] 5.2.5 Test with various object types and pool sizes

### 5.3 Scheduler Configuration
- [ ] 5.3.1 Create `SchedulerConfig<>` template
- [ ] 5.3.2 Add template parameters (max_priorities, round_robin, etc.)
- [ ] 5.3.3 Update Scheduler to use config
- [ ] 5.3.4 Test with different configurations
- [ ] 5.3.5 Document configuration options

### 5.4 Compile-Time Deadlock Detection (Optional)
- [ ] 5.4.1 Design lock dependency graph
- [ ] 5.4.2 Implement cycle detection algorithm
- [ ] 5.4.3 Add `LockDependency<MutexA, MutexB>` types
- [ ] 5.4.4 Test with intentional deadlock scenarios
- [ ] 5.4.5 Document deadlock detection usage

## Phase 6: Examples (Weeks 11-12)

### 6.1 Example: Type-Safe Producer/Consumer
- [ ] 6.1.1 Create `examples/rtos/producer_consumer/` directory
- [ ] 6.1.2 Implement producer task with type-safe queue
- [ ] 6.1.3 Implement consumer task with concept validation
- [ ] 6.1.4 Demonstrate compile-time error when types mismatch
- [ ] 6.1.5 Add README with expected behavior

### 6.2 Example: Multi-Task System
- [ ] 6.2.1 Create `examples/rtos/multi_task/` directory
- [ ] 6.2.2 Define 3 tasks with different priorities
- [ ] 6.2.3 Use TaskSet<> for compile-time registration
- [ ] 6.2.4 Show total RAM calculation at compile-time
- [ ] 6.2.5 Add README documenting memory footprint

### 6.3 Example: Task Notifications
- [ ] 6.3.1 Create `examples/rtos/task_notifications/` directory
- [ ] 6.3.2 Demonstrate ISR→Task notification
- [ ] 6.3.3 Compare footprint vs queue implementation
- [ ] 6.3.4 Show timeout handling
- [ ] 6.3.5 Add README with use case guidelines

### 6.4 Example: Memory Pools
- [ ] 6.4.1 Create `examples/rtos/memory_pools/` directory
- [ ] 6.4.2 Demonstrate type-safe allocation
- [ ] 6.4.3 Show pool exhaustion handling
- [ ] 6.4.4 Compare with static allocation
- [ ] 6.4.5 Add README with best practices

### 6.5 Example: Multi-Mutex Lock
- [ ] 6.5.1 Create `examples/rtos/multi_mutex/` directory
- [ ] 6.5.2 Demonstrate deadlock-free multi-lock
- [ ] 6.5.3 Show automatic ordering
- [ ] 6.5.4 Test with 3+ mutexes
- [ ] 6.5.5 Add README explaining pattern

### 6.6 Universal RTOS CMakeLists.txt
- [ ] 6.6.1 Create CMakeLists.txt for all RTOS examples
- [ ] 6.6.2 Support all 5 boards (F401RE, F722ZE, G071RB, G0B1RE, SAME70)
- [ ] 6.6.3 Add RTOS-specific compile definitions
- [ ] 6.6.4 Test build on all boards
- [ ] 6.6.5 Document build instructions

## Phase 7: Documentation (Week 13)

### 7.1 API Documentation
- [ ] 7.1.1 Document TaskSet<> template
- [ ] 7.1.2 Document all concepts (IPCMessage, QueueProducer, etc.)
- [ ] 7.1.3 Document Result<T, RTOSError> usage
- [ ] 7.1.4 Document TaskNotification API
- [ ] 7.1.5 Document StaticPool<> API

### 7.2 Integration Guides
- [ ] 7.2.1 Write RTOS integration guide (board porting)
- [ ] 7.2.2 Write SysTick tick source guide
- [ ] 7.2.3 Write migration guide (old API → new API)
- [ ] 7.2.4 Write type-safety patterns guide
- [ ] 7.2.5 Write performance tuning guide

### 7.3 Tutorials
- [ ] 7.3.1 Tutorial: Creating your first RTOS task
- [ ] 7.3.2 Tutorial: Type-safe inter-task communication
- [ ] 7.3.3 Tutorial: Using Result<T,E> for error handling
- [ ] 7.3.4 Tutorial: Memory management with pools
- [ ] 7.3.5 Tutorial: Advanced RTOS patterns

### 7.4 Reference Documentation
- [ ] 7.4.1 Complete API reference for all RTOS classes
- [ ] 7.4.2 Error code reference
- [ ] 7.4.3 Memory footprint reference (per-task, per-IPC object)
- [ ] 7.4.4 Timing characteristics reference
- [ ] 7.4.5 Troubleshooting reference

## Phase 8: Testing & Validation (Week 14)

### 8.1 Unit Tests
- [ ] 8.1.1 Test TaskSet with various configurations
- [ ] 8.1.2 Test concept validation (intentional errors)
- [ ] 8.1.3 Test Result<T,E> error propagation
- [ ] 8.1.4 Test TaskNotification from ISR
- [ ] 8.1.5 Test StaticPool allocation/deallocation

### 8.2 Integration Tests
- [ ] 8.2.1 Test multi-task system on all boards
- [ ] 8.2.2 Test producer/consumer with type safety
- [ ] 8.2.3 Test multi-mutex deadlock prevention
- [ ] 8.2.4 Test RTOS tick accuracy (oscilloscope validation)
- [ ] 8.2.5 Test priority inheritance with new APIs

### 8.3 Performance Tests
- [ ] 8.3.1 Measure context switch latency (should be unchanged)
- [ ] 8.3.2 Measure compile-time overhead (< 5% increase acceptable)
- [ ] 8.3.3 Measure binary size impact (< 1% increase for same functionality)
- [ ] 8.3.4 Measure RAM footprint accuracy (compile-time vs actual)
- [ ] 8.3.5 Measure interrupt latency with RTOS tick

### 8.4 Regression Tests
- [ ] 8.4.1 Verify all existing RTOS tests still pass
- [ ] 8.4.2 Verify backward compatibility layer works
- [ ] 8.4.3 Verify migration script correctness
- [ ] 8.4.4 Verify no performance regressions
- [ ] 8.4.5 Verify documentation accuracy

## Phase 9: CI/CD Integration (Week 15)

### 9.1 Build Matrix
- [ ] 9.1.1 Add RTOS examples to CI build matrix
- [ ] 9.1.2 Build all examples for all 5 boards
- [ ] 9.1.3 Add compile-time validation tests
- [ ] 9.1.4 Add concept validation tests (intentional errors should fail)
- [ ] 9.1.5 Add migration script tests

### 9.2 Hardware-in-Loop Tests
- [ ] 9.2.1 Set up HIL test infrastructure
- [ ] 9.2.2 Add RTOS tick accuracy test (oscilloscope)
- [ ] 9.2.3 Add context switch latency test
- [ ] 9.2.4 Add task execution order test
- [ ] 9.2.5 Add memory corruption test (stack overflow detection)

### 9.3 Static Analysis
- [ ] 9.3.1 Run clang-tidy on RTOS code
- [ ] 9.3.2 Run cppcheck for memory safety
- [ ] 9.3.3 Add concept diagnostic validation
- [ ] 9.3.4 Add compile-time footprint validation
- [ ] 9.3.5 Generate coverage report (target: 90%+)

## Phase 10: Release Preparation (Week 16)

### 10.1 Release Notes
- [ ] 10.1.1 Document all new features
- [ ] 10.1.2 Document breaking changes
- [ ] 10.1.3 Document migration path
- [ ] 10.1.4 Document known issues
- [ ] 10.1.5 Document future roadmap

### 10.2 Migration Support
- [ ] 10.2.1 Test migration script on 3+ real projects
- [ ] 10.2.2 Create migration examples (before/after)
- [ ] 10.2.3 Record migration video tutorial
- [ ] 10.2.4 Create migration FAQ
- [ ] 10.2.5 Set up migration support channel

### 10.3 Final Validation
- [ ] 10.3.1 Full regression test suite passes
- [ ] 10.3.2 All examples build and run on all boards
- [ ] 10.3.3 Documentation reviewed and approved
- [ ] 10.3.4 Performance metrics validated
- [ ] 10.3.5 Security review completed

### 10.4 Release
- [ ] 10.4.1 Tag release version
- [ ] 10.4.2 Publish release notes
- [ ] 10.4.3 Publish migration guide
- [ ] 10.4.4 Update project README
- [ ] 10.4.5 Announce release to community
