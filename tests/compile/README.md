# Compile Validation

`tests/compile` owns compile-time validation targets that must build for a selected device contract.

For now, the source files still live under `tests/compile_tests/` to avoid a large path churn while the runtime-validation migration is in progress. The root `CMakeLists.txt` imports [`contract_smoke.cmake`](./contract_smoke.cmake) so compile validation stays under the `tests/` taxonomy even before the physical move of source files.

This layer is responsible for:

- startup contract smoke
- board API compile checks
- driver API compile checks
- device import boundary checks

These checks are not registered as `ctest` cases because their pass/fail signal is successful compilation of the object library.
