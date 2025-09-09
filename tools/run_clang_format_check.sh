#!/usr/bin/env bash
set -euo pipefail

# Simulate the GitHub Actions clang-format check locally
# - Uses .clang-format in repo root
# - Scans tracked C/C++ source/header files

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
cd "$ROOT_DIR"

CF_BIN=${CLANG_FORMAT:-clang-format}

if ! command -v "$CF_BIN" >/dev/null 2>&1; then
  echo "ERROR: clang-format not found in PATH" >&2
  echo "Install clang-format (version 16+ recommended) and retry." >&2
  exit 127
fi

echo "clang-format: $($CF_BIN --version)"

# Files to check (tracked by git only)
FILES=()
for ext in c cc cxx cpp h hh hxx hpp; do
  while IFS= read -r -d '' f; do
    FILES+=("$f")
  done < <(git ls-files -z "*.${ext}" 2>/dev/null || true)
done

if [[ ${#FILES[@]} -eq 0 ]]; then
  echo "No C/C++ files found to check."
  exit 0
fi

echo "Checking ${#FILES[@]} files with .clang-format..."

# -n (no write), --Werror (non-zero if changes are needed)
# Use xargs -0/-d '\n' to be robust to spaces; but git ls-files won't include newlines in filenames by default.
if ! "$CF_BIN" -style=file -n --Werror "${FILES[@]}"; then
  echo
  echo "Formatting differences detected. To fix locally, run:"
  echo "  tools/run_clang_format_fix.sh"
  echo
  echo "Or manually:"
  echo "  clang-format -i \$(git ls-files '*.{c,cc,cxx,cpp,h,hh,hxx,hpp}')"
  exit 1
fi

echo "Formatting OK."
