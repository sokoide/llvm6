# Implementation Plan: Enhanced C Features Support

## Phase 1: Logic & Control Flow (demo3)
- [x] Task: Create `examples/demo3.c` demonstrating `&&`, `||`, `!`, `break`, `continue` (82a8c5a)
- [ ] Task: Implement Logical Operators (`&&`, `||`, `!`)
    - [ ] Update Lexer/Parser for `&&`, `||`, `!`
    - [ ] Update AST for logical binary/unary ops
    - [ ] Implement Codegen for logical ops (with short-circuit evaluation for `&&`, `||`)
    - [ ] Verify with unit tests
- [ ] Task: Implement `break` and `continue`
    - [ ] Update Lexer/Parser for `break`, `continue`
    - [ ] Update AST to support these statements
    - [ ] Implement Codegen (jump to loop exit/start)
- [ ] Task: Verify `demo3.c`
    - [ ] Run `make run INPUT=examples/demo3.c` and verify output
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Logic & Control Flow (demo3)' (Protocol in workflow.md)

## Phase 2: Enhanced Structs (demo4)
- [ ] Task: Create `examples/demo4.c` demonstrating nested structs and `->` operator
- [ ] Task: Implement Nested Struct Support
    - [ ] Update Type System to handle nested struct definitions
    - [ ] Update Codegen to calculate correct offsets for nested members
- [ ] Task: Implement Arrow Operator (`->`)
    - [ ] Update Lexer/Parser for `->` (if not present)
    - [ ] Update AST/Codegen to handle `ptr->member` as `(*ptr).member` equivalent
- [ ] Task: Verify `demo4.c`
    - [ ] Run `make run INPUT=examples/demo4.c` and verify output
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Enhanced Structs (demo4)' (Protocol in workflow.md)

## Phase 3: Enhanced Arrays (demo5)
- [ ] Task: Create `examples/demo5.c` demonstrating multidimensional arrays and initialization
- [ ] Task: Implement Multidimensional Arrays
    - [ ] Update Parser to handle `[N][M]` syntax
    - [ ] Update Type System for multi-level array types
    - [ ] Update Codegen for multi-dimensional index calculation
- [ ] Task: Implement Array Initialization
    - [ ] Update Parser for `{...}` initializer lists
    - [ ] Update Codegen to generate `store` instructions or `memcpy` for initialization
- [ ] Task: Verify `demo5.c`
    - [ ] Run `make run INPUT=examples/demo5.c` and verify output
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Enhanced Arrays (demo5)' (Protocol in workflow.md)
