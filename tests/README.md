# Test Suite Documentation

## Test Organization

### 📁 **fixtures/**
Input C source files for testing the compiler. Each file tests specific language features:

- **Basic Features**: `simple.c`, `constants.c`, `variables.c`
- **Functions**: `functions.c`, `function.c`, `recursive.c`, `multiple_functions.c`
- **Control Flow**: `control_flow.c`, `statements.c`
- **Expressions**: `expressions.c`, `arithmetic.c`
- **Complex Features**: `arrays_pointers.c`, `types.c`, `complex.c`

### 📁 **output/**
Generated LLVM IR files (.ll) from test compilation. Files are automatically created during test runs.

### 📁 **reports/**
Test results and coverage analysis:
- Coverage reports (.gcov files)
- LCOV coverage summaries
- Test execution logs

## Running Tests

### Quick Tests
```bash
make test-quick     # Basic functionality validation
```

### Comprehensive Testing
```bash
make test           # Full test suite
make test-verbose   # Detailed output with source code
```

### Validation
```bash
make validate       # LLVM IR syntax validation
make test-coverage  # Code coverage analysis
```

## Test Categories

1. **Syntax Tests**: Basic C language constructs
2. **Semantic Tests**: Type checking and symbol resolution
3. **Code Generation**: LLVM IR output correctness
4. **Edge Cases**: Error handling and boundary conditions

## Adding New Tests

1. Create test file in `fixtures/` with `.c` extension
2. Run `make test` to generate corresponding `.ll` output
3. Verify output correctness manually
4. Add to version control for regression testing