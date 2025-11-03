# Community Custom SVDs

This directory contains community-contributed SVD files for custom boards or devices not officially supported by vendors.

## Purpose

Use this directory for:
- Custom board SVD files
- Modified vendor SVDs with fixes
- Community-created device descriptions
- Prototype/development board SVDs

## Contributing

When adding SVD files here:

1. **Naming**: Use descriptive names (e.g., `CustomBoard_v1.svd`)
2. **Documentation**: Add entry below with full details
3. **Testing**: Verify code generation works correctly
4. **License**: Clearly state the license (prefer open-source licenses)

## Files

(Document your community SVD files here)

### Example Entry:
<!--
- **MyCustomBoard.svd**
  - Description: Custom STM32F103-based development board
  - Author: Your Name
  - Date Added: 2025-11-03
  - Based On: STM32F103C8.svd (modified)
  - Changes: Added custom peripheral definitions
  - License: MIT
  - Contact: email@example.com
-->

## Guidelines

### Quality Standards
- SVD must be valid XML
- Must include at least GPIO and one peripheral
- Should follow CMSIS-SVD specification
- Test with actual hardware when possible

### Documentation Requirements
- Clear description of the device/board
- Author/maintainer contact information
- Base SVD (if derived from another)
- List of modifications made
- Testing status

### License
- Prefer MIT, BSD, or Apache 2.0 licenses
- Clearly state license in SVD file header
- Respect original vendor licenses if derived

## Support

For community-contributed SVDs:
- Support is best-effort
- Contact the original author for issues
- Report issues in project issue tracker
- PRs for fixes are welcome

## Disclaimer

Community SVD files may not be as well-tested as vendor-provided files. Use at your own risk and verify with hardware testing.
