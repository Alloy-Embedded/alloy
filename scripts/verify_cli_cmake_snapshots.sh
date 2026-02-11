#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TMP_DIR="${TMPDIR:-/tmp}/microcore-regression-snapshots"

mkdir -p "${TMP_DIR}"
rm -rf "${TMP_DIR}/build-f401" "${TMP_DIR}/build-legacy"

strip_ansi() {
  sed -E 's/\x1b\[[0-9;]*m//g'
}

check_contains() {
  local file="$1"
  local needle="$2"
  if ! grep -Fq "${needle}" "${file}"; then
    echo "Snapshot mismatch: expected '${needle}' in ${file}" >&2
    echo "---- ${file} ----" >&2
    cat "${file}" >&2
    exit 1
  fi
}

cd "${ROOT_DIR}"

echo "[snapshot] ucore --help"
./ucore --help | strip_ansi > "${TMP_DIR}/ucore_help.txt"
check_contains "${TMP_DIR}/ucore_help.txt" "build"
check_contains "${TMP_DIR}/ucore_help.txt" "flash"
check_contains "${TMP_DIR}/ucore_help.txt" "smoke"
check_contains "${TMP_DIR}/ucore_help.txt" "list"

echo "[snapshot] ucore list boards"
./ucore list boards | strip_ansi > "${TMP_DIR}/ucore_list_boards.txt"
check_contains "${TMP_DIR}/ucore_list_boards.txt" "nucleo_f401re"
check_contains "${TMP_DIR}/ucore_list_boards.txt" "nucleo_f722ze"
check_contains "${TMP_DIR}/ucore_list_boards.txt" "nucleo_g071rb"
check_contains "${TMP_DIR}/ucore_list_boards.txt" "nucleo_g0b1re"
check_contains "${TMP_DIR}/ucore_list_boards.txt" "same70_xplained"

echo "[snapshot] ucore list examples"
./ucore list examples | strip_ansi > "${TMP_DIR}/ucore_list_examples.txt"
check_contains "${TMP_DIR}/ucore_list_examples.txt" "blink"
check_contains "${TMP_DIR}/ucore_list_examples.txt" "rtos/simple_tasks"
check_contains "${TMP_DIR}/ucore_list_examples.txt" "api_tiers/simple_gpio_blink"
check_contains "${TMP_DIR}/ucore_list_examples.txt" "api_tiers/simple_gpio_button"

echo "[snapshot] cmake configure (canonical vars)"
cmake \
  -B "${TMP_DIR}/build-f401" \
  -DMICROCORE_BOARD=nucleo_f401re \
  -DMICROCORE_BUILD_TESTS=OFF \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake 2>&1 \
  | strip_ansi > "${TMP_DIR}/cmake_configure_microcore.txt"

check_contains "${TMP_DIR}/cmake_configure_microcore.txt" "Platform auto-detected: stm32f4 (from board: nucleo_f401re)"
check_contains "${TMP_DIR}/cmake_configure_microcore.txt" "Board: nucleo_f401re (validated against platform stm32f4)"

check_contains "${TMP_DIR}/build-f401/CMakeCache.txt" "MICROCORE_BOARD:STRING=nucleo_f401re"
check_contains "${TMP_DIR}/build-f401/CMakeCache.txt" "MICROCORE_PLATFORM:STRING=stm32f4"

echo "[snapshot] cmake build blink"
cmake --build "${TMP_DIR}/build-f401" --target blink -- -j2 > "${TMP_DIR}/cmake_build_blink.txt" 2>&1
if [[ ! -f "${TMP_DIR}/build-f401/examples/blink/blink" ]]; then
  echo "Snapshot mismatch: expected built artifact build-f401/examples/blink/blink" >&2
  exit 1
fi

echo "[snapshot] cmake configure (legacy aliases)"
cmake \
  -B "${TMP_DIR}/build-legacy" \
  -DALLOY_BOARD=nucleo_f401re \
  -DALLOY_BUILD_TESTS=OFF \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake 2>&1 \
  | strip_ansi > "${TMP_DIR}/cmake_configure_legacy.txt"

check_contains "${TMP_DIR}/cmake_configure_legacy.txt" "CMake Deprecation Warning"
check_contains "${TMP_DIR}/cmake_configure_legacy.txt" "ALLOY_BOARD is deprecated"
check_contains "${TMP_DIR}/build-legacy/CMakeCache.txt" "MICROCORE_BOARD:STRING=nucleo_f401re"

echo "[snapshot] cmake build blink (legacy aliases)"
cmake --build "${TMP_DIR}/build-legacy" --target blink -- -j2 > "${TMP_DIR}/cmake_build_blink_legacy.txt" 2>&1
if [[ ! -f "${TMP_DIR}/build-legacy/examples/blink/blink" ]]; then
  echo "Snapshot mismatch: expected built artifact build-legacy/examples/blink/blink" >&2
  exit 1
fi

echo "CLI/CMake regression snapshots: OK"
