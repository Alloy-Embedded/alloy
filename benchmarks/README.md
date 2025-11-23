# MicroCore Benchmarks

Performance benchmarks for MicroCore HAL framework, measuring real-world performance metrics and comparing against other embedded frameworks.

## Overview

MicroCore benchmarks measure:

- **GPIO Toggle Frequency** - Maximum pin toggle rate
- **Interrupt Latency** - Time from IRQ trigger to handler execution
- **Context Switch Time** - RTOS task switching overhead
- **UART Throughput** - Serial communication bandwidth
- **SPI Throughput** - SPI transfer rates

## Building Benchmarks

```bash
# Build all benchmarks for a board
./ucore build nucleo_f401re benchmarks

# Build specific benchmark
cmake --build build-nucleo_f401re --target gpio_toggle_bench
cmake --build build-nucleo_f401re --target interrupt_latency_bench
```

## Running Benchmarks

### GPIO Toggle Benchmark

Measures the maximum GPIO toggle frequency achievable with zero-overhead abstractions.

**Hardware Setup:**
1. Connect oscilloscope probe to PA5 (LED pin)
2. Flash benchmark to board
3. Measure toggle frequency on scope

**Running:**
```bash
./ucore flash nucleo_f401re gpio_toggle_bench
```

**Expected Results:**

| Platform | CPU | MicroCore | Arduino | mbed | STM32 HAL |
|----------|-----|-----------|---------|------|-----------|
| STM32F401RE | 84 MHz | ~5-8 MHz | ~100 kHz | ~500 kHz | ~1 MHz |
| STM32F722ZE | 216 MHz | ~15-20 MHz | ~100 kHz | ~800 kHz | ~2 MHz |
| SAME70Q21 | 300 MHz | ~20-30 MHz | N/A | ~1 MHz | ~3 MHz |

**Analysis:**
- **Direct register:** Theoretical maximum (1 toggle per 6-10 cycles)
- **MicroCore HAL:** Near-zero overhead (1-2 cycles added)
- **Other frameworks:** 10-100x slower due to abstraction layers

### Interrupt Latency Benchmark

Measures interrupt response time from trigger to handler execution.

**Hardware Setup:**
1. Connect scope CH1 to PA5 (trigger pin)
2. Connect scope CH2 to PA6 (response pin)
3. Flash benchmark to board
4. Measure time between rising edges

**Running:**
```bash
./ucore flash nucleo_f401re interrupt_latency_bench
```

**Expected Results:**

| Platform | CPU | Latency (cycles) | Latency (ns) | Comparison |
|----------|-----|------------------|--------------|------------|
| STM32F401RE | 84 MHz | 12-20 | 150-250 ns | Bare-metal |
| STM32F722ZE | 216 MHz | 12-20 | 55-90 ns | Bare-metal |

**Comparison with other frameworks:**
- **Arduino:** ~10-20 µs (50-100x slower)
- **mbed RTOS:** ~2-5 µs (10-20x slower)
- **STM32 HAL:** ~1-2 µs (5-10x slower)
- **MicroCore:** <500 ns (near bare-metal)

### Context Switch Benchmark

*(Future implementation)*

Measures RTOS task switching overhead for real-time systems.

### UART Throughput Benchmark

*(Future implementation)*

Measures serial communication bandwidth with DMA.

### SPI Throughput Benchmark

*(Future implementation)*

Measures SPI transfer rates with DMA.

## Benchmark Methodology

### Timing Measurement

All benchmarks use hardware cycle counters for precise measurement:

**Cortex-M3/M4/M7:**
- DWT (Debug Watch and Trace) cycle counter
- Resolution: 1 CPU cycle
- Accuracy: ±1 cycle

**Cortex-M0/M0+:**
- SysTick fallback (less accurate)
- Resolution: 24-bit downcounter
- Accuracy: ±2-3 cycles

### Optimization Levels

Benchmarks are compiled with maximum optimization:
```cmake
-O3              # Maximum speed optimization
-flto            # Link-time optimization
-fno-exceptions  # No exception overhead
-fno-rtti        # No RTTI overhead
```

This matches production build settings and demonstrates real-world performance.

### Statistical Analysis

Each benchmark runs multiple iterations to compute:
- **Average:** Mean performance across all runs
- **Best case:** Minimum cycles observed
- **Worst case:** Maximum cycles observed
- **Std deviation:** Performance variance

Warmup iterations are excluded from measurements to ensure CPU caches are primed.

## Comparison Framework

### Target Platforms

Benchmarks compare MicroCore against:

1. **Arduino** - Widely-used beginner framework
2. **mbed** - ARM's official embedded OS
3. **STM32 HAL** - ST's vendor HAL
4. **Zephyr RTOS** - Linux Foundation RTOS
5. **Direct register access** - Theoretical maximum

### Performance Metrics

- **Cycles per operation** - Raw CPU cycles
- **Time per operation** - Real-time (µs/ms)
- **Frequency** - Operations per second (Hz/kHz/MHz)
- **Throughput** - Bytes per second (for I/O)
- **Overhead** - Abstraction cost vs direct register access

### Baseline Values

MicroCore targets **zero-overhead** abstraction:
- GPIO operations: 1-2 cycles overhead vs direct register
- Interrupt latency: Hardware minimum (12-20 cycles)
- Context switch: <100 cycles (with RTOS)

## Results Reporting

### Console Output

Benchmarks print results in human-readable format:

```
========================================
MicroCore Benchmark Results
========================================

GPIO Toggle Frequency
---------------------
Platform:     STM32F401RE (Cortex-M4 @ 84 MHz)
Iterations:   100,000
Total cycles: 1,200,000
Cycles/iter:  12 cycles
Time/iter:    143 ns
Frequency:    7.0 MHz

Comparison with baseline:
  Direct register:  10 cycles (baseline)
  MicroCore HAL:    12 cycles (+20% overhead)
  Arduino:          8400 cycles (+84000% overhead)

Result: ✓ PASS (within 50% of baseline)
```

### CSV Export

For automated analysis, results can be exported to CSV:

```csv
benchmark,platform,cpu_mhz,cycles,time_ns,frequency_hz,overhead_pct
gpio_toggle,stm32f401re,84,12,143,7000000,20
interrupt_latency,stm32f401re,84,18,214,N/A,80
```

### Oscilloscope Verification

Hardware measurements using oscilloscope provide independent verification:
- Measure actual pin toggle frequency
- Measure actual interrupt response time
- Compare with software measurements

Any discrepancy >5% indicates measurement error or CPU throttling.

## CI/CD Integration

*(Future implementation)*

Benchmarks will run automatically in CI:

```yaml
# .github/workflows/benchmarks.yml
- name: Run benchmarks on hardware
  run: |
    ./ucore flash nucleo_f401re gpio_toggle_bench
    python3 scripts/collect_benchmark_results.py
    python3 scripts/compare_with_baseline.py

- name: Upload results
  uses: actions/upload-artifact@v4
  with:
    name: benchmark-results
    path: benchmark_results.csv
```

CI will fail if performance degrades >10% vs baseline.

## Adding New Benchmarks

To add a new benchmark:

1. **Create benchmark file** (`my_bench.cpp`):
```cpp
#include "benchmark.hpp"

void bench_my_feature() {
    BenchmarkConfig config{};
    config.iterations = 10000;

    auto result = run_benchmark("My feature", [] {
        // Code to benchmark
    }, config);
}

int main() {
    init_cycle_counter();
    bench_my_feature();
    return 0;
}
```

2. **Add to CMakeLists.txt**:
```cmake
add_executable(my_bench my_bench.cpp)
target_link_libraries(my_bench PRIVATE microcore_hal)
```

3. **Document expected results** in this README

4. **Add baseline values** for comparison

## Performance Goals

MicroCore aims for:

- ✅ GPIO operations: <20 cycles (0-2 cycle overhead)
- ✅ Interrupt latency: <50 cycles (hardware minimum)
- ⏳ Context switch: <100 cycles (with RTOS)
- ⏳ UART throughput: >1 Mbps (with DMA)
- ⏳ SPI throughput: >10 Mbps (with DMA)

## Troubleshooting

### No benchmark output

**Problem:** Benchmark runs but no output visible
**Solution:** Connect UART console or use debugger to view results

### Incorrect cycle counts

**Problem:** Cycle counts seem too low/high
**Solution:**
- Check CPU frequency setting in `benchmark.hpp`
- Verify DWT is enabled (Cortex-M3/M4/M7)
- Ensure no CPU throttling or sleep modes

### Oscilloscope shows different frequency

**Problem:** Scope measurement doesn't match software
**Solution:**
- Check scope probe compensation
- Verify CPU is running at expected frequency
- Check for bus matrix contention

## Related Documentation

- [Performance Guide](../docs/PERFORMANCE.md) - Optimization techniques
- [API Reference](../docs/api/) - HAL API documentation
- [Contributing](../CONTRIBUTING.md) - Development workflow

## License

See [LICENSE](../LICENSE) for details.
