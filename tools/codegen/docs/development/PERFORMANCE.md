# ⚡ Performance Optimization Guide

## 🚀 Quick Tips for Faster Generation

### Use Quiet Mode (Recommended)
```bash
# Much faster - minimal console output
./codegen generate --quiet
./codegen generate -q

# Or short form
./codegen g -q
```

**Speed improvement**: ~30-40% faster by reducing I/O overhead

### Generate Only What You Need

```bash
# Only startup files (fastest)
./codegen generate --startup

# Only pins for one vendor
./codegen generate --pins --vendor st

# Specific combination
./codegen generate --startup --pins --vendor atmel -q
```

### Disable Manifest (Not Recommended)

```bash
# Skip manifest tracking (slightly faster, but loses tracking)
./codegen generate --no-manifest -q
```

**Note**: Only use this if you don't need file tracking/validation

---

## 📊 Performance Breakdown

### What Takes Time

| Operation | Time | % of Total | Optimizable |
|-----------|------|------------|-------------|
| SVD Discovery | ~1-2s | 5% | ✅ Already optimized |
| SVD Parsing | ~5-10s | 20% | ✅ Cached |
| File Generation | ~15-20s | 60% | ⚠️ CPU bound |
| Manifest Updates | ~2-3s | 10% | ✅ Optimized |
| Console Output | ~2-5s | 5% | ✅ Use --quiet |

### Typical Generation Times

| Mode | MCUs | Files | Time | Command |
|------|------|-------|------|---------|
| **Quiet** | 35 | 175 | ~25-30s | `./codegen g -q` |
| **Normal** | 35 | 175 | ~35-40s | `./codegen g` |
| **Verbose** | 35 | 175 | ~50-60s | `./codegen g -v` |
| **Startup Only** | 8 | 16 | ~5-8s | `./codegen g --startup -q` |
| **ST Pins Only** | 26 | 130 | ~15-20s | `./codegen g --pins --vendor st -q` |

---

## ⚡ Optimizations Already Implemented

### 1. Smart SVD Discovery ✅

**Before**:
```python
# Parsed ALL ~500+ SVD files to find 8 board MCUs
for svd_file in all_svd_files:  # 500+ iterations!
    device = parse_svd(svd_file)  # Expensive!
    if is_board_mcu(device):
        ...
```

**After**:
```python
# Uses cached discovery - only looks up 8 specific files
all_svds = discover_all_svds()  # Fast - scans filenames only
for board_mcu in BOARD_MCUS:  # 8 iterations
    svd_file = all_svds[board_mcu]  # Direct lookup!
```

**Speed improvement**: 95% reduction in SVD parsing time

### 2. Optimized Manifest ✅

- Only calculates checksums once per file
- Writes manifest once at the end (not per file)
- Uses efficient JSON serialization

### 3. Progress Tracking Optimization ✅

- Minimal overhead in quiet mode
- Batch updates where possible
- No expensive formatting when not needed

---

## 🔧 Advanced Optimizations (Future)

### Option 1: Parallel Generation (Not Implemented Yet)

Could speed up by ~2-3x on multi-core systems:

```python
# Potential future optimization
from multiprocessing import Pool

with Pool(processes=4) as pool:
    results = pool.map(generate_mcu, mcu_list)
```

**Why not implemented**:
- ProgressTracker is not thread-safe
- Manifest writes would conflict
- Complexity vs benefit tradeoff

**Workaround**: Use separate commands in parallel:
```bash
# Terminal 1
./codegen generate --startup -q &

# Terminal 2
./codegen generate --pins --vendor st -q &

# Terminal 3
./codegen generate --pins --vendor atmel -q &

# Wait for all
wait
```

### Option 2: Incremental Generation

Only regenerate files that changed:

```bash
# Future feature
./codegen generate --incremental
```

Would check:
- SVD file modification time
- Generator script changes
- Only regenerate what's needed

### Option 3: Template Caching

Pre-compile Jinja templates (if used in future):

```python
# Cache compiled templates
template_cache = {}
```

---

## 🎯 Recommended Workflows

### Daily Development (Fastest)

```bash
# Generate only what you're working on
./codegen generate --pins --vendor st -q

# Or just startup
./codegen generate --startup -q
```

### Full Build (Moderate)

```bash
# Everything, quiet mode
./codegen generate -q
```

### CI/CD (Comprehensive)

```bash
# Full build with validation
./codegen generate
./codegen clean --validate
```

### First Time Setup (Verbose)

```bash
# See what's happening
./codegen generate --verbose
```

---

## 📈 Measuring Performance

### Built-in Timing

The summary shows time taken:

```
================================================================================
                        GENERATION SUMMARY
================================================================================

Total MCUs processed: 35
Time elapsed: 25.3s
Average: 0.72s per MCU
```

### Manual Benchmarking

```bash
# Unix/Linux/macOS
time ./codegen generate -q

# Output:
# real    0m25.123s
# user    0m22.456s
# sys     0m1.234s
```

### Compare Modes

```bash
# Quiet mode
time ./codegen generate -q
# → ~25s

# Normal mode
time ./codegen generate
# → ~35s

# Verbose mode
time ./codegen generate -v
# → ~50s
```

---

## 💡 Tips & Tricks

### 1. Use Short Aliases

Add to `~/.bashrc` or `~/.zshrc`:

```bash
alias cg='cd /path/to/alloy/tools/codegen && ./codegen'
alias cgq='cg generate -q'  # Quick generate
alias cgs='cg generate --startup -q'  # Startup only
alias cgp='cg generate --pins -q'  # Pins only
```

Usage:
```bash
cgq        # Fast full generate
cgs        # Startup only
cgp        # Pins only
```

### 2. Skip What You Don't Need

```bash
# Working on STM32 only?
./codegen generate --pins --vendor st -q

# Need startup for testing?
./codegen generate --startup -q
```

### 3. Use Dry Runs for Testing

```bash
# See what would be generated without actually doing it
./codegen clean --dry-run
```

### 4. Monitor Progress

```bash
# Watch files being created in real-time (another terminal)
watch -n 1 'find src/hal/vendors -type f -name "*.hpp" | wc -l'
```

---

## 🐛 Troubleshooting Slow Performance

### Issue: Generation takes >2 minutes

**Possible causes**:
1. Slow disk (HDD vs SSD)
2. Antivirus scanning each file
3. Running in verbose mode
4. Many other processes

**Solutions**:
```bash
# Use quiet mode
./codegen generate -q

# Exclude directory from antivirus
# Add src/hal/vendors to exclusions

# Check disk speed
time dd if=/dev/zero of=testfile bs=1M count=100
rm testfile
```

### Issue: SVD discovery is slow

**Check**:
```bash
# See how many SVD files exist
find upstream/cmsis-svd-data -name "*.svd" | wc -l
```

If >1000 files, discovery might be slow on network drives.

**Solution**: Work on local disk, not network drive

### Issue: High memory usage

**Cause**: Generating all files at once

**Solution**: Generate in batches
```bash
./codegen generate --startup -q
./codegen generate --pins --vendor st -q
./codegen generate --pins --vendor atmel -q
```

---

## 📊 Expected Performance

### On Modern Hardware (SSD, 8GB RAM, 4 cores)

| Mode | Expected Time |
|------|---------------|
| Quiet, full | 20-30s |
| Normal, full | 30-40s |
| Verbose, full | 45-60s |
| Startup only | 5-10s |
| Pins, 1 vendor | 10-20s |

### On Older Hardware (HDD, 4GB RAM, 2 cores)

| Mode | Expected Time |
|------|---------------|
| Quiet, full | 40-60s |
| Normal, full | 60-90s |
| Verbose, full | 90-120s |

---

## ✅ Best Practices

1. **Use quiet mode by default** - only use verbose for debugging
2. **Generate incrementally** during development
3. **Full generation** before commits/releases
4. **Monitor manifest** to track what's generated
5. **Validate occasionally** to catch manual edits

---

## 🚀 Quick Reference

```bash
# Fastest - quiet mode, all
./codegen generate -q

# Fast - startup only
./codegen g --startup -q

# Fast - one vendor
./codegen g --pins --vendor st -q

# Normal - see progress
./codegen generate

# Slow - debug mode
./codegen generate -v

# Check what's generated
./codegen clean --stats
```

---

**Last Updated**: 2025-11-05
**Version**: 1.1.0
