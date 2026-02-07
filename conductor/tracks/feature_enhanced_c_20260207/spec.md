# Specification: Enhanced C Features Support (Logic, Structs, Arrays)

## Goal
Extend the Tiny-C compiler to support currently unsupported C language features and syntax. This will be demonstrated through the creation and successful compilation/execution of `demo3.c`, `demo4.c`, and `demo5.c`.

## Scope & Roadmap

### 1. Logic & Control Flow (`demo3.c`)
*   **Logical Operators**: Implement `&&` (AND), `||` (OR), and `!` (NOT).
*   **Control Flow**: Implement `break` and `continue` statements within loops.
*   **Comparison**: Ensure all standard comparison operators (`==`, `!=`, `<`, `<=`, `>`, `>=`) work correctly in complex expressions.

### 2. Enhanced Structs (`demo4.c`)
*   **Nested Structs**: Support defining struct members that are themselves structs.
*   **Arrow Operator**: Fully support `ptr->member` syntax for accessing members of struct pointers (including nested access like `ptr->sub.val`).

### 3. Enhanced Arrays (`demo5.c`)
*   **Multidimensional Arrays**: Support declaration and access of 2D+ arrays (e.g., `int grid[3][3]`).
*   **Initialization**: Support array initialization syntax (e.g., `int arr[] = {1, 2, 3};`).

## Success Criteria
1.  **Demo Compilation**: `examples/demo3.c`, `examples/demo4.c`, and `examples/demo5.c` must compile to LLVM IR without errors.
2.  **Correct Execution**: The compiled binaries must execute and produce the expected output (verified via `make run`).
3.  **Test Coverage**: New features must be covered by unit tests in `tests/unit/`.

## Out of Scope (for this track)
- Floating point types (`float`, `double`).
- Switch-case statements.
- Do-while loops.
