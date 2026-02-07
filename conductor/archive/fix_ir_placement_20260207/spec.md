# Specification: Fix LLVM IR Global Constant Placement Bug

## Goal
Fix a bug where string constants are being placed inside LLVM functions, causing `llc` to fail with "expected instruction opcode". Global constants must be defined at the module level (outside functions).

## Requirements
1. **Global Scope Placement**: Move the generation of LLVM global constants (like string literals) to the module level.
2. **Reference from Function**: Ensure that functions correctly reference these global constants using their IDs.
3. **Compatibility**: Maintain existing string literal functionality.

## Success Criteria
- `make run-ir INPUT=examples/demo2.ll` passes without "expected instruction opcode" error.
- String literals are printed correctly in the resulting binary.
- All existing tests pass.
