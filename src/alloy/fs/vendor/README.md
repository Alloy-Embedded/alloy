# Vendored: littlefs

Verbatim, unmodified copy of [littlefs](https://github.com/littlefs-project/littlefs)
— a fail-safe, wear-levelling filesystem for microcontrollers.

- **Version:** v2.11.3
- **Commit:** `6cb4e86540eca0d9ba62500a298385c9d863c8be`
- **License:** BSD-3-Clause (see `LICENSE.md`)
- **Files:** `lfs.c`, `lfs.h`, `lfs_util.c`, `lfs_util.h`, `LICENSE.md`

These are the first vendored C sources in the tree. They are framework-owned
(not header-only), so `tools/alloy/alloy_cli/build.py` compiles them **only for
boards that declare an `fs` role** (detected from the generated `board.hpp`),
with warnings silenced (`-w`) and, on firmware, the lean config
`LFS_NO_MALLOC LFS_NO_ASSERT LFS_NO_DEBUG LFS_NO_WARN LFS_NO_ERROR` — the
alloy facade (`alloy/fs/littlefs.hpp`) supplies all littlefs buffers statically,
so no heap is used.

## Updating

Refetch all five files at the desired tag and update the version/commit above:

```sh
SHA=<new-commit-sha>
for f in lfs.c lfs.h lfs_util.c lfs_util.h LICENSE.md; do
  curl -sL "https://raw.githubusercontent.com/littlefs-project/littlefs/$SHA/$f" \
    -o "src/alloy/fs/vendor/$f"
done
```

Do not edit the sources locally — keep them a clean mirror so updates stay a
drop-in. All alloy-specific behavior lives in `alloy/fs/littlefs.hpp`.
