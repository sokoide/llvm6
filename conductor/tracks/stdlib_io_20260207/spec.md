# Specification: Standard Library Integration - Basic I/O Support

## Goal
Enable the compiler to support basic I/O operations by allowing declaration and linking of external functions from the C standard library (specifically `printf` and `scanf`).

## Requirements
1. **External Function Declaration**: Support `extern` keyword or implicit external declarations for functions defined in the standard library.
2. **Function Signature Matching**: Ensure function calls match the declared external signatures.
3. **LLVM IR Generation**: Generate `declare` statements in LLVM IR for external functions.
4. **Linking**: Ensure the generated object files or IR can be linked with `libc`.

## Success Criteria
- The compiler can parse a C file containing a `printf` call.
- The generated LLVM IR contains `declare i32 @printf(i8*, ...)`.
- Executing the compiled binary correctly prints output to the console.
- Test coverage for new features is >80%.
