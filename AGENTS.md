# Repository Guidelines

## Project Structure & Module Organization
The core compiler lives in `src/`, split into `lexer.l`, `grammar.y`, `ast.*`, `codegen.*`, and `main.cpp` for the driver. Generated artifacts and object files land in `build/`, while the assembled binary is published to `ccompiler/ccompiler`. Documentation lives under `docs/` with deeper architecture notes, and `tests/` is organized into `fixtures/` (input C files), `output/` (generated `.ll`), and `reports/` for coverage data. Avoid committing anything from `build/` or `tests/output/`; they are reproducible.

## Build, Test, and Development Commands
Run `make check-deps` once to confirm `bison`, `flex`, `g++`, and optional LLVM tools. `make` (or `make debug`) compiles the driver and stages artifacts in `build/`. Use `make run INPUT=hello.c OUTPUT=hello.ll` to translate a fixture without manual flag juggling. `make test` executes the full regression suite; `make test-quick` is the fast smoke run; `make test-coverage` emits LCOV data under `tests/reports/`; `make validate` feeds generated IR through `llvm-as` for syntax checking.

## Coding Style & Naming Conventions
Follow the established C++-17 style: tab-indented blocks (rendered as four spaces), braces on their own lines, and descriptive `snake_case` identifiers. Use `ALL_CAPS` for enum constants, token names, and configuration macros in `constants.h`. Keep grammar actions in `grammar.y` concise and prefer `/* ... */` block comments for parser and lexer changes. There is no enforced formatter, so mirror the surrounding file before submitting.

## Testing Guidelines
Add new acceptance cases to `tests/fixtures/feature_name.c`; the suite expects the matching `.ll` to appear in `tests/output/` after running `make test`. Keep debug-oriented samples in `tests/debug_*` with paired IR snapshots. Aim to preserve the current 97% pass rate and include regressions for any parser or IR bug you fix. When reproducing a failure, capture the compiler invocation and any `make` target you used in the PR notes.

## Commit & Pull Request Guidelines
Commits use the short prefix convention from `git log`: `add:`, `fix:`, `improve:`, `refactor`, etc., followed by an imperative description (e.g., `fix: handle pointer dereference in codegen`). Bundle logical changes together and re-run `make test` before committing. Pull requests should include: 1) a summary of the behavior change, 2) reference to an issue or motivation, 3) test evidence (`make test` output or new fixtures), and 4) when relevant, a brief IR snippet or AST dump that highlights the change. Small screenshots of coverage diffs go in `tests/reports/` and can be attached as PR artifacts.
