# Repository Guidelines

This repository is intentionally minimal. Use the conventions below when adding code so the project stays consistent and easy to build, test, and review.

## Project Structure & Module Organization

Suggested layout (C++/CMake):

```
./CMakeLists.txt           # top-level build
./src/                     # implementation (.cpp)
./include/minfi/           # public headers (.hpp)
./tests/                   # unit tests (gtest)
./assets/                  # small sample inputs (kept lightweight)
./bench/                   # micro/throughput benchmarks (optional)
```

Namespaces: use `minfi::...`. Filenames: `snake_case.cpp/.hpp`. Keep headers in `include/minfi/` mirroring namespaces.

## Build, Test, and Development Commands

- Configure and build (Release by default):
  - `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
  - `cmake --build build -j`
- Run unit tests:
  - `ctest --test-dir build --output-on-failure`
- Run an example/binary (if present):
  - `./build/bin/minfi_demo --help`

If you add new targets or tests, wire them into `CMakeLists.txt` and `add_subdirectory(tests)`.

## Coding Style & Naming Conventions

- Language: C++17 or later (prefer C++20 features where available).
- Indentation: 2 spaces; no tabs. Line length ≤ 100.
- Headers: `#pragma once`; prefer `.hpp` for C++ headers.
- Formatting: clang-format (Google style as baseline). Run: `clang-format -i $(git ls-files "*.{cpp,hpp,h}")`.
- Linting: enable warnings-as-errors for project code (`-Wall -Wextra -Werror -Wpedantic`).
- API design: pass by `const&` for large types; use `string_view`, `span`, and `gsl`-style checks where helpful.

## Testing Guidelines

- Framework: GoogleTest via CTest.
- File naming: `*_test.cpp`; test fixtures mirror `src/` structure.
- Coverage: target ≥ 80% for core algorithms. Optional: integrate `gcovr` and expose `make coverage` or a CTest dashboard.
- Keep tests fast and deterministic; include tiny sample frames in `assets/` only.

## Commit & Pull Request Guidelines

- Commits: follow Conventional Commits (`feat:`, `fix:`, `perf:`, `refactor:`, `test:`, `docs:`). Keep messages imperative and scoped.
- PRs must include: clear description, rationale, screenshots or before/after frame diffs for visual changes, and linked issues.
- Add benchmarks or complexity notes for algorithmic changes; document trade-offs in code comments where relevant.

## Security & Configuration Tips

- Do not commit large datasets or proprietary models; prefer small, synthetic samples. Use Git LFS for any binaries if needed.
- Keep secrets out of the repo. Place machine-specific paths behind CMake options (e.g., `-DMINFI_WITH_CUDA=ON`).

## Agent-Specific Instructions

- When modifying files, preserve the structure above and update `CMakeLists.txt` and tests together. Prefer small, focused diffs and explain build changes in PRs.

