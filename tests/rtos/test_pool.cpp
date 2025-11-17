/// Unit tests for RTOS StaticPool

#include "tests/rtos/test_framework.hpp"
#include "rtos/memory_pool.hpp"

using namespace alloy;
using namespace alloy::rtos;
using namespace alloy::rtos::test;

// Test types
struct SmallBlock {
    core::u32 value;
};

struct MediumBlock {
    core::u32 data[16];
};

static_assert(PoolAllocatable<SmallBlock>);
static_assert(PoolAllocatable<MediumBlock>);

TEST_SUITE(StaticPool) {
    TEST_CASE(pool_construction) {
        StaticPool<SmallBlock, 8> pool;

        TEST_ASSERT_EQUAL(pool.available(), 8);
        TEST_ASSERT_EQUAL(pool.capacity(), 8);
        TEST_ASSERT(pool.is_full());
        TEST_ASSERT(!pool.is_empty());

        // Compile-time checks
        static_assert(pool.capacity() == 8);
        static_assert(pool.block_size() == sizeof(SmallBlock));
    }

    TEST_CASE(pool_allocate_deallocate) {
        StaticPool<SmallBlock, 4> pool;

        // Allocate
        auto result = pool.allocate();
        TEST_ASSERT_OK(result);

        SmallBlock* block = result.unwrap();
        TEST_ASSERT(block != nullptr);

        TEST_ASSERT_EQUAL(pool.available(), 3);

        // Use block
        block->value = 42;
        TEST_ASSERT_EQUAL(block->value, 42);

        // Deallocate
        auto dealloc_result = pool.deallocate(block);
        TEST_ASSERT_OK(dealloc_result);

        TEST_ASSERT_EQUAL(pool.available(), 4);
    }

    TEST_CASE(pool_exhaust) {
        StaticPool<SmallBlock, 4> pool;

        SmallBlock* blocks[4];

        // Allocate all blocks
        for (int i = 0; i < 4; i++) {
            auto result = pool.allocate();
            TEST_ASSERT_OK(result);
            blocks[i] = result.unwrap();
        }

        TEST_ASSERT(pool.is_empty());
        TEST_ASSERT_EQUAL(pool.available(), 0);

        // Try to allocate from empty pool
        auto result = pool.allocate();
        TEST_ASSERT_ERR(result);
        TEST_ASSERT_EQUAL(result.unwrap_err(), RTOSError::NoMemory);

        // Deallocate all
        for (int i = 0; i < 4; i++) {
            TEST_ASSERT_OK(pool.deallocate(blocks[i]));
        }

        TEST_ASSERT(pool.is_full());
    }

    TEST_CASE(pool_fifo_order) {
        StaticPool<SmallBlock, 4> pool;

        SmallBlock* blocks[4];

        // Allocate all
        for (int i = 0; i < 4; i++) {
            blocks[i] = pool.allocate().unwrap();
            blocks[i]->value = i;
        }

        // Deallocate in reverse order
        for (int i = 3; i >= 0; i--) {
            pool.deallocate(blocks[i]).unwrap();
        }

        // Re-allocate (should be LIFO for free list)
        SmallBlock* new_blocks[4];
        for (int i = 0; i < 4; i++) {
            new_blocks[i] = pool.allocate().unwrap();
        }

        // All allocations should succeed
        TEST_ASSERT_EQUAL(pool.available(), 0);
    }

    TEST_CASE(pool_invalid_pointer) {
        StaticPool<SmallBlock, 4> pool;

        SmallBlock external_block;

        // Try to deallocate pointer not from pool
        auto result = pool.deallocate(&external_block);
        TEST_ASSERT_ERR(result);
        TEST_ASSERT_EQUAL(result.unwrap_err(), RTOSError::InvalidPointer);
    }

    TEST_CASE(pool_reset) {
        StaticPool<SmallBlock, 4> pool;

        // Allocate some blocks
        auto b1 = pool.allocate().unwrap();
        auto b2 = pool.allocate().unwrap();

        TEST_ASSERT_EQUAL(pool.available(), 2);

        // Reset pool (WARNING: only safe when no blocks in use!)
        pool.reset();

        TEST_ASSERT_EQUAL(pool.available(), 4);
        TEST_ASSERT(pool.is_full());
    }

    TEST_CASE(pool_multiple_allocations) {
        StaticPool<MediumBlock, 8> pool;

        constexpr int num_iterations = 100;
        constexpr int blocks_per_iter = 4;

        for (int iter = 0; iter < num_iterations; iter++) {
            // Allocate
            MediumBlock* blocks[blocks_per_iter];
            for (int i = 0; i < blocks_per_iter; i++) {
                auto result = pool.allocate();
                TEST_ASSERT_OK(result);
                blocks[i] = result.unwrap();

                // Use the block
                for (int j = 0; j < 16; j++) {
                    blocks[i]->data[j] = iter * 100 + i * 10 + j;
                }
            }

            // Verify data
            for (int i = 0; i < blocks_per_iter; i++) {
                for (int j = 0; j < 16; j++) {
                    TEST_ASSERT_EQUAL(blocks[i]->data[j],
                                      iter * 100 + i * 10 + j);
                }
            }

            // Deallocate
            for (int i = 0; i < blocks_per_iter; i++) {
                TEST_ASSERT_OK(pool.deallocate(blocks[i]));
            }

            TEST_ASSERT_EQUAL(pool.available(), 8);
        }
    }

    TEST_CASE(pool_performance_allocate) {
        StaticPool<SmallBlock, 16> pool;

        PerfTimer timer;

        // Measure 1000 allocate/deallocate cycles
        for (int i = 0; i < 1000; i++) {
            auto block = pool.allocate().unwrap();
            pool.deallocate(block).unwrap();
        }

        core::u32 elapsed = timer.elapsed_us();
        core::u32 avg = elapsed / 1000;

        printf("\n    Average alloc+dealloc: %lu µs\n", avg);

        // Should be very fast (<1µs)
        TEST_ASSERT_IN_RANGE(avg, 0, 2);
    }

    TEST_CASE(pool_allocator_raii) {
        StaticPool<SmallBlock, 4> pool;

        TEST_ASSERT_EQUAL(pool.available(), 4);

        {
            // RAII allocation
            PoolAllocator<SmallBlock> alloc1(pool);
            TEST_ASSERT(alloc1.is_valid());
            TEST_ASSERT_EQUAL(pool.available(), 3);

            alloc1->value = 123;
            TEST_ASSERT_EQUAL(alloc1->value, 123);

            {
                PoolAllocator<SmallBlock> alloc2(pool);
                TEST_ASSERT(alloc2.is_valid());
                TEST_ASSERT_EQUAL(pool.available(), 2);

                // Both allocations active
            }

            // alloc2 deallocated
            TEST_ASSERT_EQUAL(pool.available(), 3);
        }

        // alloc1 deallocated
        TEST_ASSERT_EQUAL(pool.available(), 4);
        TEST_ASSERT(pool.is_full());
    }

    TEST_CASE(pool_allocator_move) {
        StaticPool<SmallBlock, 4> pool;

        PoolAllocator<SmallBlock> alloc1(pool);
        TEST_ASSERT(alloc1.is_valid());
        alloc1->value = 999;

        // Move constructor
        PoolAllocator<SmallBlock> alloc2(std::move(alloc1));
        TEST_ASSERT(!alloc1.is_valid());  // Moved from
        TEST_ASSERT(alloc2.is_valid());
        TEST_ASSERT_EQUAL(alloc2->value, 999);

        // Only one allocation active
        TEST_ASSERT_EQUAL(pool.available(), 3);
    }

    TEST_CASE(pool_allocator_release) {
        StaticPool<SmallBlock, 4> pool;

        PoolAllocator<SmallBlock> alloc(pool);
        TEST_ASSERT(alloc.is_valid());

        SmallBlock* raw_ptr = alloc.release();
        TEST_ASSERT(raw_ptr != nullptr);
        TEST_ASSERT(!alloc.is_valid());

        // Manual deallocation required
        pool.deallocate(raw_ptr).unwrap();

        TEST_ASSERT_EQUAL(pool.available(), 4);
    }

    TEST_CASE(pool_compile_time_validation) {
        // Optimal capacity calculation
        constexpr size_t capacity = optimal_pool_capacity<SmallBlock, 1024>();
        StaticPool<SmallBlock, capacity> pool;

        // Should fit in budget
        static_assert(pool.total_size() <= 1024);

        printf("\n    Optimal capacity for 1KB: %zu blocks\n", capacity);
    }

    TEST_CASE(pool_budget_validation) {
        // Verify pool fits in budget
        static_assert(pool_fits_budget<SmallBlock, 16, 2048>(),
                      "Pool should fit in 2KB");

        StaticPool<SmallBlock, 16> pool;
        printf("\n    Pool size: %zu bytes\n", pool.total_size());

        TEST_ASSERT(pool.total_size() <= 2048);
    }

    TEST_CASE(pool_stress_test) {
        StaticPool<MediumBlock, 8> pool;

        // Random allocation/deallocation pattern
        MediumBlock* active_blocks[4] = {nullptr};

        for (int iter = 0; iter < 1000; iter++) {
            int slot = iter % 4;

            if (active_blocks[slot] != nullptr) {
                // Deallocate
                pool.deallocate(active_blocks[slot]).unwrap();
                active_blocks[slot] = nullptr;
            } else {
                // Allocate
                auto result = pool.allocate();
                if (result.is_ok()) {
                    active_blocks[slot] = result.unwrap();

                    // Use block
                    for (int i = 0; i < 16; i++) {
                        active_blocks[slot]->data[i] = iter + i;
                    }
                }
            }
        }

        // Clean up
        for (int i = 0; i < 4; i++) {
            if (active_blocks[i] != nullptr) {
                pool.deallocate(active_blocks[i]).unwrap();
            }
        }

        TEST_ASSERT(pool.is_full());
    }
}
