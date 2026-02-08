# Implementation Plan: Self-hosting Tiny-C Compiler

## Phase 1: Diagnostic and Basic Expression Support [checkpoint: f94307d]
コンパイラ自身のソースコードをパースし、最初のエラーを特定・解決する。また、基本的なビット演算子などを実装する。

- [x] Task: Attempt to self-compile and list prioritized missing features/errors
- [x] Task: Implement bitwise operators (`&`, `|`, `^`, `~`, `<<`, `>>`)
- [x] Task: Implement compound assignment operators (`+=`, `&=`, etc.)
- [x] Task: Verify expression support with existing `examples/` and new test fixtures
- [x] Task: Conductor - User Manual Verification 'Phase 1' (Protocol in workflow.md)

## Phase 2: Type System Enhancements (Typedef and Enum) [checkpoint: 0b05059]
データ構造の定義に不可欠な `typedef` と `enum` を実装する。

- [x] Task: Implement `enum` parsing and value assignment in symbol table
- [x] Task: Implement `typedef` support (type aliasing)
- [x] Task: Handle basic system types (size_t, FILE, etc.) through simple headers
- [x] Task: Verify type aliasing logic with complex declaration tests
- [x] Task: Conductor - User Manual Verification 'Phase 2' (Protocol in workflow.md)

## Phase 3: Structural Support (Struct and Union) [checkpoint: 0b05059]
ASTやシンボルテーブルの内部構造を扱えるよう、構造体と共用体を完全にサポートする。

- [x] Task: Implement full struct/union member layout and alignment logic
- [x] Task: Support nested structs and anonymous structs/unions (if needed)
- [x] Task: Implement arrow operator (`->`) and member access code generation
- [x] Task: Verify structural access with unit tests and complex fixtures
- [x] Task: Conductor - User Manual Verification 'Phase 3' (Protocol in workflow.md)

## Phase 4: Advanced Logic and Function Pointers
セルフホストに必要な残りの論理機能と関数ポインタを実装する。

- [ ] Task: Implement conditional operator (`? :`)
- [ ] Task: Implement function pointer declarations and calls
- [ ] Task: Enhance system header compatibility (ignoring/processing specific attributes)
- [ ] Task: Verify full parser compatibility with a preprocessed `src/main.c`
- [ ] Task: Conductor - User Manual Verification 'Phase 4' (Protocol in workflow.md)

## Phase 5: Bootstrapping and Parity Validation
2段階ビルドを実行し、出力の整合性を検証する。

- [ ] Task: Generate `tc2` (Tiny-C compiled by Tiny-C)
- [ ] Task: Verify `tc2` can compile the compiler source to produce `tc3`
- [ ] Task: Compare `tc2` output and `tc3` output for functional parity
- [ ] Task: Conductor - User Manual Verification 'Phase 5' (Protocol in workflow.md)
