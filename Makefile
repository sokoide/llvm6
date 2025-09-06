# C to LLVM IR Compiler Makefile
TARGET = ./ccompiler
CXX = g++
CC = gcc
CXXFLAGS = -Wall -Wextra -O2 -std=c++11 -Isrc
CFLAGS = -Wall -Wextra -O2
LIBS = -ly

# Source files
SOURCES = src/main.cpp src/ast.cpp src/codegen.cpp grammar.tab.cpp lex.yy.c
OBJECTS = main.o ast.o codegen.o grammar.tab.o lex.yy.o

# Generated files
GENERATED = grammar.tab.cpp grammar.tab.hpp lex.yy.c grammar.output

# LLVM configuration
LLVM_CONFIG = llvm-config
LLVM_CXXFLAGS = $(shell $(LLVM_CONFIG) --cppflags 2>/dev/null || echo "")
LLVM_LDFLAGS = $(shell $(LLVM_CONFIG) --ldflags 2>/dev/null || echo "")
LLVM_LIBS = $(shell $(LLVM_CONFIG) --libs core 2>/dev/null || echo "")

# Add LLVM flags if available
ifneq ($(LLVM_CXXFLAGS),)
	CXXFLAGS += $(LLVM_CXXFLAGS)
	LDFLAGS += $(LLVM_LDFLAGS)
	LIBS += $(LLVM_LIBS)
endif

# Default target
all: $(TARGET)

# Build the compiler
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS) $(LIBS)

# Object file dependencies
main.o: src/main.cpp src/ast.h src/codegen.h grammar.tab.hpp
	$(CXX) $(CXXFLAGS) -c src/main.cpp -o main.o

ast.o: src/ast.cpp src/ast.h
	$(CXX) $(CXXFLAGS) -c src/ast.cpp -o ast.o

codegen.o: src/codegen.cpp src/codegen.h src/ast.h
	$(CXX) $(CXXFLAGS) -c src/codegen.cpp -o codegen.o

grammar.tab.o: grammar.tab.cpp src/ast.h src/codegen.h
	$(CXX) $(CXXFLAGS) -c grammar.tab.cpp -o grammar.tab.o

lex.yy.o: lex.yy.c grammar.tab.hpp
	$(CC) $(CFLAGS) -c lex.yy.c -o lex.yy.o

# Generate parser from grammar
grammar.tab.cpp grammar.tab.hpp: src/grammar.y
	@echo "Generating parser..."
	bison -d -v -o grammar.tab.cpp src/grammar.y

# Generate lexer from specification
lex.yy.c: src/lexer.l grammar.tab.hpp
	@echo "Generating lexer..."
	flex src/lexer.l

# Clean generated and object files
clean:
	@echo "Cleaning generated files..."
	rm -f $(TARGET) *.o $(GENERATED)

# Clean everything including backup files and coverage data
distclean: clean clean-coverage
	rm -f *~ *.bak core

# Run the compiler (requires input)
run: $(TARGET)
	@echo "Usage: make run INPUT=file.c [OUTPUT=file.ll]"
	@if [ -z "$(INPUT)" ]; then \
		echo "Error: INPUT file not specified"; \
		echo "Example: make run INPUT=test.c OUTPUT=test.ll"; \
		exit 1; \
	fi
	@if [ -n "$(OUTPUT)" ]; then \
		$(TARGET) $(INPUT) -o $(OUTPUT); \
	else \
		$(TARGET) $(INPUT); \
	fi

# Debug build with symbols and debug info
debug: CXXFLAGS += -g -DDEBUG -O0
debug: CFLAGS += -g -DDEBUG -O0
debug: $(TARGET)

# Comprehensive test suite
test: $(TARGET)
	@echo "Running comprehensive compiler test suite..."
	@echo "====================================================="
	@failed=0; total=0; \
	for test_file in tests/*.c; do \
		if [ -f "$$test_file" ]; then \
			total=$$((total + 1)); \
			test_name=$$(basename "$$test_file" .c); \
			echo "Testing $$test_name..."; \
			if $(TARGET) "$$test_file" -o "tests/$$test_name.ll" 2>/dev/null; then \
				echo "  ✓ $$test_name: PASSED"; \
			else \
				echo "  ✗ $$test_name: FAILED"; \
				failed=$$((failed + 1)); \
			fi; \
		fi; \
	done; \
	echo "====================================================="; \
	echo "Test Results: $$((total - failed))/$$total passed"; \
	if [ $$failed -gt 0 ]; then \
		echo "⚠️  $$failed test(s) failed"; \
		exit 1; \
	else \
		echo "🎉 All tests passed!"; \
	fi

# Detailed test with verbose output
test-verbose: $(TARGET)
	@echo "Running tests with verbose output..."
	@for test_file in tests/*.c; do \
		if [ -f "$$test_file" ]; then \
			test_name=$$(basename "$$test_file" .c); \
			echo "=== Testing $$test_name ==="; \
			echo "Source:"; \
			head -5 "$$test_file"; \
			echo "Output:"; \
			$(TARGET) "$$test_file" -o "tests/$$test_name.ll" -v || true; \
			echo ""; \
		fi; \
	done

# Quick test of basic functionality
test-quick: $(TARGET)
	@echo "Running quick functionality tests..."
	@$(TARGET) tests/simple.c -o tests/simple.ll >/dev/null 2>&1 && echo "✓ Basic compilation works" || echo "✗ Basic compilation failed"
	@$(TARGET) tests/variables.c >/dev/null 2>&1 && echo "✓ Variable handling works" || echo "✗ Variable handling failed"
	@$(TARGET) tests/functions.c >/dev/null 2>&1 && echo "✓ Function parsing works" || echo "✗ Function parsing failed"

# Validate generated LLVM IR
validate: test
	@echo "Validating LLVM IR..."
	@for file in tests/*.ll; do \
		echo "Validating $$file..."; \
		llvm-as "$$file" -o /dev/null 2>/dev/null && echo "✓ $$file is valid" || echo "✗ $$file has errors"; \
	done

# Code coverage analysis
test-coverage: clean
	@echo "Building with coverage instrumentation..."
	@$(MAKE) CXXFLAGS="$(CXXFLAGS) --coverage -fprofile-arcs -ftest-coverage" \
	         CFLAGS="$(CFLAGS) --coverage -fprofile-arcs -ftest-coverage" \
	         LDFLAGS="$(LDFLAGS) --coverage" \
	         $(TARGET)
	@echo "Running tests for coverage analysis..."
	@$(MAKE) test-quick 2>/dev/null || true
	@echo "Generating coverage report..."
	@mkdir -p coverage
	@gcov -o . src/*.cpp src/*.c 2>/dev/null || echo "gcov not available, skipping detailed coverage"
	@echo "Coverage files generated:"
	@ls -la *.gcov 2>/dev/null || echo "No .gcov files found"
	@echo "Coverage summary:"
	@if command -v lcov >/dev/null 2>&1; then \
		lcov --capture --directory . --output-file coverage/coverage.info 2>/dev/null && \
		lcov --summary coverage/coverage.info 2>/dev/null; \
	else \
		echo "lcov not available, install with: brew install lcov (macOS) or apt-get install lcov (Linux)"; \
	fi
	@echo "Coverage analysis complete. Check .gcov files for line-by-line coverage."

# Clean coverage files
clean-coverage:
	@echo "Cleaning coverage files..."
	@rm -f *.gcov *.gcda *.gcno
	@rm -rf coverage/

# Install the compiler (requires sudo)
install: $(TARGET)
	@echo "Installing $(TARGET) to /usr/local/bin..."
	sudo cp $(TARGET) /usr/local/bin/
	@echo "Installation complete"

# Uninstall the compiler
uninstall:
	@echo "Removing $(TARGET) from /usr/local/bin..."
	sudo rm -f /usr/local/bin/ccompiler
	@echo "Uninstallation complete"

# Show help
help:
	@echo "Available targets:"
	@echo "  all            - Build the compiler (default)"
	@echo "  clean          - Remove generated and object files"
	@echo "  distclean      - Remove all generated files including backups and coverage"
	@echo "  run            - Run the compiler (specify INPUT=file.c)"
	@echo "  debug          - Build with debug symbols"
	@echo ""
	@echo "Testing targets:"
	@echo "  test           - Run comprehensive test suite on all test files"
	@echo "  test-verbose   - Run tests with detailed output"
	@echo "  test-quick     - Run basic functionality tests"
	@echo "  test-coverage  - Run tests with code coverage analysis"
	@echo "  validate       - Validate generated LLVM IR syntax"
	@echo "  clean-coverage - Remove coverage analysis files"
	@echo ""
	@echo "Installation:"
	@echo "  install        - Install compiler system-wide"
	@echo "  uninstall      - Remove system-wide installation"
	@echo "  check-deps     - Check for required dependencies"
	@echo "  help           - Show this help"
	@echo ""
	@echo "Examples:"
	@echo "  make run INPUT=program.c OUTPUT=program.ll"
	@echo "  make debug && ./ccompiler -v -a program.c"
	@echo "  make test && make validate"
	@echo "  make test-coverage    # Generate code coverage report"

# Check dependencies
check-deps:
	@echo "Checking dependencies..."
	@which bison >/dev/null || (echo "✗ bison not found" && exit 1)
	@which flex >/dev/null || (echo "✗ flex not found" && exit 1)
	@which $(CXX) >/dev/null || (echo "✗ $(CXX) not found" && exit 1)
	@echo "✓ All dependencies found"
	@if which $(LLVM_CONFIG) >/dev/null 2>&1; then \
		echo "✓ LLVM found: $$($(LLVM_CONFIG) --version)"; \
	else \
		echo "⚠ LLVM not found - some features may be limited"; \
	fi

.PHONY: all clean distclean run debug test test-verbose test-quick test-coverage validate install uninstall help check-deps clean-coverage