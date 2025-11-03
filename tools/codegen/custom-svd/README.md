# Custom SVD Repository

This directory allows you to add SVD (System View Description) files from vendors or community sources that are not included in the upstream `cmsis-svd-data` repository.

## Directory Structure

```
custom-svd/
├── README.md (this file)
├── merge_policy.json (SVD merge configuration)
└── vendors/
    ├── STMicro/       (STMicroelectronics devices)
    ├── NXP/           (NXP devices)
    ├── Atmel/         (Microchip/Atmel devices)
    └── Community/     (Community-contributed devices)
```

## Adding Custom SVD Files

### 1. File Naming Convention
- Use the official device name (e.g., `STM32F103C8.svd`, `LPC1768.svd`)
- Keep the `.svd` extension
- Use consistent capitalization as per vendor documentation

### 2. Vendor Directory
Place your SVD file in the appropriate vendor directory:

```bash
# Example: Adding an STM32 device
cd tools/codegen/custom-svd/vendors/STMicro
wget https://example.com/path/to/STM32F103C8.svd

# Example: Adding a community device
cd tools/codegen/custom-svd/vendors/Community
cp ~/my-custom-board.svd CustomBoard.svd
```

### 3. Vendor README (Required)
Each vendor directory should have a README.md with source information:

```markdown
# STMicroelectronics Custom SVDs

## Source
- Downloaded from: https://www.st.com/resource/en/svd/...
- Date: 2025-11-03
- License: BSD-3-Clause (or applicable license)

## Files
- STM32F103C8.svd - 64KB variant (LQFP48 package)
  - Source: ST official website
  - Version: 1.2
  - Reason: Upstream version outdated

- STM32F103CB.svd - 128KB variant (LQFP48 package)
  - Source: ST official website
  - Version: 1.2
  - Reason: Not available in upstream
```

## Merge Priority

Custom SVD files have **higher priority** than upstream files. If the same device exists in both locations:

- ✅ **Custom SVD** will be used (from `custom-svd/`)
- ⚠️ Warning will be logged showing which file was selected
- 📋 Upstream SVD will be ignored

Example:
```
⚠️  WARNING: Duplicate SVD detected for STM32F103C8
    - Upstream: tools/codegen/upstream/cmsis-svd-data/STMicro/STM32F103C8.svd
    - Custom:   tools/codegen/custom-svd/vendors/STMicro/STM32F103C8.svd

    Using: custom-svd (higher priority)
```

## Validation

Before using custom SVDs, they are automatically validated:

### XML Syntax
- Must be valid XML
- Must have proper opening/closing tags
- Must be well-formed

### Required Fields
- `<device><name>` - Device name
- `<device><addressUnitBits>` - Address unit bits
- `<device><width>` - Register width
- `<device><peripherals>` - At least one peripheral

### Example Validation Error
```
❌ ERROR: Invalid SVD file
   File: custom-svd/vendors/STMicro/BadDevice.svd
   Issue: XML parsing error at line 42: Missing closing tag for <peripheral>
   Action: Fix XML syntax or remove file
```

## Running Code Generation

After adding custom SVD files:

```bash
# Regenerate all MCU code (including custom SVDs)
make codegen

# Or manually:
cd tools/codegen
python update_all_vendors.py
python generate_all.py
```

## Listing Available SVDs

To see all SVD files (both upstream and custom):

```bash
make list-svds
```

Output example:
```
📦 Available SVD Files:

Upstream (cmsis-svd-data):
  ✓ STM32F103C6.svd
  ✓ STM32F103CB.svd
  ... (848 more)

Custom (custom-svd/):
  ✓ STMicro/STM32F103C8.svd  [Overrides upstream]
  ✓ STMicro/STM32F103VE.svd  [New device]
  ✓ Community/CustomBoard.svd

Total: 850 SVDs (848 upstream, 3 custom, 1 override)
```

## Contributing Guidelines

### For Personal/Project Use
- Add any SVD files you need
- Document the source in vendor README
- Commit to your repository

### For Upstream Contribution
If you have SVDs that should be in the main repository:

1. **Check if it's already upstream**: Search `upstream/cmsis-svd-data/`
2. **Verify quality**: Ensure SVD is complete and tested
3. **Document source**: Include vendor URL and license
4. **Create PR**: Submit to [cmsis-svd-data](https://github.com/posborne/cmsis-svd) repository

### Best Practices
- ✅ Always document SVD source and version
- ✅ Test generated code before committing
- ✅ Keep vendor READMEs up to date
- ✅ Use official vendor SVDs when possible
- ❌ Don't commit SVDs with unclear licensing
- ❌ Don't modify upstream SVD files directly

## Troubleshooting

### Problem: SVD file not detected
**Solution**: Ensure it's in a vendor subdirectory (not directly in `custom-svd/`)

### Problem: Compilation errors after adding SVD
**Solution**: Run validation to check SVD quality:
```bash
python tools/codegen/validate_svds.py custom-svd/vendors/YourVendor/YourDevice.svd
```

### Problem: Conflicting with upstream SVD
**Solution**: This is expected. Custom SVDs override upstream. Check the warning message to confirm correct file is used.

### Problem: Missing peripheral definitions
**Solution**: SVD file may be incomplete. Check vendor documentation or use upstream version.

## File Format

SVD files are XML-based following the CMSIS-SVD standard:

```xml
<?xml version="1.0" encoding="utf-8"?>
<device schemaVersion="1.3">
  <name>STM32F103C8</name>
  <version>1.2</version>
  <description>STM32F103C8 - ARM Cortex-M3 MCU</description>
  <addressUnitBits>8</addressUnitBits>
  <width>32</width>

  <peripherals>
    <peripheral>
      <name>GPIOA</name>
      <!-- ... registers ... -->
    </peripheral>
  </peripherals>
</device>
```

## Additional Resources

- [CMSIS-SVD Specification](https://arm-software.github.io/CMSIS_5/SVD/html/index.html)
- [CMSIS-SVD Repository](https://github.com/posborne/cmsis-svd)
- [Alloy Codegen Documentation](../README.md)
- [MCU Support Guide](../../docs/MCU_SUPPORT.md)

## License

Custom SVD files retain their original licenses. Check each vendor's README for specific license information.
