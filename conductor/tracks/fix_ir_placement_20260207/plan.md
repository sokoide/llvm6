# Implementation Plan: Fix LLVM IR Global Constant Placement Bug

## Phase 1: Diagnosis & Reproduction
- [ ] Task: Reproduce the bug with `examples/demo2.ll`
- [ ] Task: Identify the code responsible for emitting global constants within functions in `src/codegen.cpp`
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Diagnosis' (Protocol in workflow.md)

## Phase 2: Fix Implementation
- [ ] Task: Refactor `CodegenContext` or `Codegen` to handle module-level constants
    - [ ] Separate constant emission from function body emission
- [ ] Task: Update `visit(StringLiteralAST*)` to register constants for module-level emission
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Fix Implementation' (Protocol in workflow.md)

## Phase 3: Verification
- [ ] Task: Run integration test `examples/demo2.ll`
- [ ] Task: Run full test suite to ensure no regressions
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Verification' (Protocol in workflow.md)
