#!/usr/bin/env bash
set -euo pipefail

# Apply clang-format to all tracked C/C++ files

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
cd "$ROOT_DIR"

CF_BIN=${CLANG_FORMAT:-clang-format}

if ! command -v "$CF_BIN" >/dev/null 2>&1; then
  echo "ERROR: clang-format not found in PATH" >&2
  echo "Install clang-format (version 16+ recommended) and retry." >&2
  exit 127
fi

FILES=()
for ext in c cc cxx cpp h hh hxx hpp; do
  while IFS= read -r -d '' f; do
    FILES+=("$f")
  done < <(git ls-files -z "*.${ext}" 2>/dev/null || true)
done

if [[ ${#FILES[@]} -eq 0 ]]; then
  echo "No C/C++ files found to format."
  exit 0
fi

echo "Formatting ${#FILES[@]} files with $($CF_BIN --version)"
"$CF_BIN" -style=file -i "${FILES[@]}"

echo "Done. Review changes with: git diff -- ."
