# LLVM C Compiler Documentation

This directory contains comprehensive documentation for the LLVM C compiler project.

## Documentation Structure

### 📁 **architecture/**
- System design and architectural patterns
- Component relationships and data flow
- Clean architecture compliance analysis

### 📁 **api/**
- API documentation for core components
- Function signatures and usage examples
- Symbol tables and type system reference

### 📁 **examples/**
- Code examples and usage patterns
- Test case explanations
- Best practices and coding standards

## Quick Start

For project overview and development guidance, see [`CLAUDE.md`](./CLAUDE.md).

## Project Structure

```
claude/
├── docs/           # Documentation (this directory)
├── srccpp/            # Source code
├── tests/          # Test cases organized by type
│   ├── fixtures/   # Input test files (.c)
│   ├── output/     # Generated LLVM IR (.ll)
│   └── reports/    # Coverage and validation reports
├── build/          # Build artifacts and generated files
└── Makefile        # Build system with structured output
```

## Key Commands

```bash
make test           # Run comprehensive test suite
make test-coverage  # Generate code coverage reports
make validate       # Validate LLVM IR output
make help          # Show all available commands
```