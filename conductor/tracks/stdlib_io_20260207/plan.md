# Implementation Plan: Standard Library Integration - Basic I/O Support

## Phase 1: Infrastructure & External Declarations
- [ ] Task: Update Lexer and Parser for `extern` keyword (if not already fully supported)
    - [ ] Add `EXTERN` token to `lexer.l`
    - [ ] Update `grammar.y` to handle external declarations
- [ ] Task: Update AST to represent external functions
    - [ ] Add `ExternalFunctionDeclaration` node to `ast.h/cpp`
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Infrastructure' (Protocol in workflow.md)

## Phase 2: Codegen & Linking
- [ ] Task: Implement LLVM IR generation for external declarations
    - [ ] Update `codegen.cpp` to emit `declare` for external functions
- [ ] Task: Verify integration with simple `printf` example
    - [ ] Write a test case in `examples/` using `printf`
    - [ ] Manually verify the output after compilation
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Codegen & Linking' (Protocol in workflow.md)

## Phase 3: Testing & Cleanup
- [ ] Task: Comprehensive Unit Tests
    - [ ] Write unit tests for external function parsing in `tests/unit/test_ast.cpp`
    - [ ] Write unit tests for external function codegen in `tests/unit/test_codegen_utils.cpp`
- [ ] Task: Integration Tests
    - [ ] Add integration tests in `tests/fixtures/` for various stdlib functions
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Testing & Cleanup' (Protocol in workflow.md)
