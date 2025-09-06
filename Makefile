# C to LLVM IR Compiler Makefile
TARGET = ./ccompiler
CXX = g++
CC = gcc
CXXFLAGS = -Wall -Wextra -O2 -std=c++11
CFLAGS = -Wall -Wextra -O2
LIBS = -ly

# Source files
SOURCES = main.cpp ast.cpp codegen.cpp grammar.tab.cpp lex.yy.c
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
main.o: main.cpp ast.h codegen.h grammar.tab.hpp
	$(CXX) $(CXXFLAGS) -c main.cpp -o main.o

ast.o: ast.cpp ast.h
	$(CXX) $(CXXFLAGS) -c ast.cpp -o ast.o

codegen.o: codegen.cpp codegen.h ast.h
	$(CXX) $(CXXFLAGS) -c codegen.cpp -o codegen.o

grammar.tab.o: grammar.tab.cpp ast.h codegen.h
	$(CXX) $(CXXFLAGS) -c grammar.tab.cpp -o grammar.tab.o

lex.yy.o: lex.yy.c grammar.tab.hpp
	$(CC) $(CFLAGS) -c lex.yy.c -o lex.yy.o

# Generate parser from grammar
grammar.tab.cpp grammar.tab.hpp: grammar.y
	@echo "Generating parser..."
	bison -d -v -o grammar.tab.cpp grammar.y

# Generate lexer from specification
lex.yy.c: lexer.l grammar.tab.hpp
	@echo "Generating lexer..."
	flex lexer.l

# Clean generated and object files
clean:
	@echo "Cleaning generated files..."
	rm -f $(TARGET) *.o $(GENERATED)

# Clean everything including backup files
distclean: clean
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

# Create test directory and sample files
test-setup:
	@echo "Creating test environment..."
	@mkdir -p tests
	@echo 'int main() { return 42; }' > tests/simple.c
	@echo 'int add(int a, int b) { return a + b; } int main() { return add(5, 3); }' > tests/function.c
	@echo 'int factorial(int n) { if (n <= 1) return 1; return n * factorial(n-1); } int main() { return factorial(5); }' > tests/recursive.c

# Run tests
test: $(TARGET) test-setup
	@echo "Running compiler tests..."
	@echo "=== Test 1: Simple program ==="
	$(TARGET) tests/simple.c -o tests/simple.ll -v
	@echo "=== Test 2: Function call ==="
	$(TARGET) tests/function.c -o tests/function.ll -v
	@echo "=== Test 3: Recursive function ==="
	$(TARGET) tests/recursive.c -o tests/recursive.ll -v -a
	@echo "All tests completed"

# Validate generated LLVM IR
validate: test
	@echo "Validating LLVM IR..."
	@for file in tests/*.ll; do \
		echo "Validating $$file..."; \
		llvm-as "$$file" -o /dev/null 2>/dev/null && echo "✓ $$file is valid" || echo "✗ $$file has errors"; \
	done

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
	@echo "  all       - Build the compiler (default)"
	@echo "  clean     - Remove generated and object files"
	@echo "  distclean - Remove all generated files including backups"
	@echo "  run       - Run the compiler (specify INPUT=file.c)"
	@echo "  debug     - Build with debug symbols"
	@echo "  test      - Run test suite"
	@echo "  validate  - Validate generated LLVM IR"
	@echo "  install   - Install compiler system-wide"
	@echo "  uninstall - Remove system-wide installation"
	@echo "  help      - Show this help"
	@echo ""
	@echo "Examples:"
	@echo "  make run INPUT=program.c OUTPUT=program.ll"
	@echo "  make debug && ./ccompiler -v -a program.c"
	@echo "  make test && make validate"

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

.PHONY: all clean distclean run debug test test-setup validate install uninstall help check-deps