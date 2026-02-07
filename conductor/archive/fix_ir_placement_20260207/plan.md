# Implementation Plan: Fix LLVM IR Global Constant Placement Bug

## Phase 1: Diagnosis & Reproduction
- [x] Task: Reproduce the bug with `examples/demo2.ll` (d4a4a7f)
- [x] Task: Identify the code responsible for emitting global constants within functions in `src/codegen.cpp` (d4a4a7f)
- [x] Task: Conductor - User Manual Verification 'Phase 1: Diagnosis' (Protocol in workflow.md)

## Phase 2: Fix Implementation
- [x] Task: Refactor `CodegenContext` or `Codegen` to handle module-level constants (d4a4a7f)
    - [x] Separate constant emission from function body emission (d4a4a7f)
- [x] Task: Update `visit(StringLiteralAST*)` to register constants for module-level emission (d4a4a7f)
- [x] Task: Conductor - User Manual Verification 'Phase 2: Fix Implementation' (Protocol in workflow.md)

## Phase 3: Verification
- [x] Task: Run integration test `examples/demo2.ll` (d4a4a7f)
- [x] Task: Run full test suite to ensure no regressions (d4a4a7f)
- [x] Task: Conductor - User Manual Verification 'Phase 3: Verification' (Protocol in workflow.md)
