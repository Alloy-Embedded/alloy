# STMicroelectronics Custom SVDs

This directory contains custom SVD files for STM32 microcontrollers that are either not available in upstream or need to be updated.

## Source

All SVD files in this directory should be sourced from official STMicroelectronics resources.

- **Official ST Website**: https://www.st.com/
- **STM32 Cube**: https://www.st.com/en/development-tools/stm32cubemx.html
- **GitHub**: https://github.com/STMicroelectronics

## License

STMicroelectronics SVD files are typically licensed under BSD-3-Clause or proprietary ST license. Check individual file headers for specific license information.

## Files

(Add your custom SVD files here and document them below)

### Example Entry:
<!--
- **STM32F103C8.svd**
  - Description: 64KB flash variant, LQFP48 package
  - Source: https://www.st.com/resource/en/svd/...
  - Version: 1.2
  - Date Added: 2025-11-03
  - Reason: Upstream version outdated / not available
  - License: BSD-3-Clause
-->

## Adding Files

When adding new SVD files to this directory:

1. Download from official ST sources
2. Verify the SVD is for the correct device variant
3. Document the file in this README using the template above
4. Test code generation: `make codegen`

## Notes

- STM32 devices have many variants (C6, C8, CB, etc.) - ensure you use the correct SVD
- Package differences (LQFP48, LQFP64, etc.) affect available GPIO pins
- Check ST's official SVD repository first before adding custom files
