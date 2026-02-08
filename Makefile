# Tiny-C Compiler Makefile (Pure C Version)
TARGET = ./ccompiler_c
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -Isrc
LIBS =

# Directory structure
BUILD_DIR = build
SRC_DIR = src
TEST_DIR = tests/unit
TEST_FIXTURES = tests/fixtures
TEST_OUTPUT = tests/output

# Source files
C_SOURCES = src/main.c src/memory.c src/error.c src/ast.c src/symbols.c src/codegen.c $(BUILD_DIR)/grammar_c.tab.c $(BUILD_DIR)/lex_c.yy.c
C_OBJECTS = $(BUILD_DIR)/c_main.o $(BUILD_DIR)/c_memory.o $(BUILD_DIR)/c_error.o $(BUILD_DIR)/c_ast.o $(BUILD_DIR)/c_symbols.o $(BUILD_DIR)/c_codegen.o $(BUILD_DIR)/c_grammar.o $(BUILD_DIR)/c_lex.o

# Unit tests
C_TEST_BINARIES = $(BUILD_DIR)/test_memory_c $(BUILD_DIR)/test_error_c $(BUILD_DIR)/test_ast_c $(BUILD_DIR)/test_enum_c $(BUILD_DIR)/test_typedef_c $(BUILD_DIR)/test_struct_c $(BUILD_DIR)/test_member_access_c

# Default target
all: $(TARGET)

# Build the compiler
$(TARGET): $(BUILD_DIR) $(C_OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CC) $(CFLAGS) -o $(TARGET) $(C_OBJECTS) $(LIBS)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TEST_OUTPUT):
	mkdir -p $(TEST_OUTPUT)

# Object file dependencies
$(BUILD_DIR)/c_main.o: src/main.c src/common.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

$(BUILD_DIR)/c_memory.o: src/memory.c src/memory.h src/common.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

$(BUILD_DIR)/c_error.o: src/error.c src/error.h src/common.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

$(BUILD_DIR)/c_ast.o: src/ast.c src/ast.h src/common.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

$(BUILD_DIR)/c_symbols.o: src/symbols.c src/symbols.h src/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

$(BUILD_DIR)/c_codegen.o: src/codegen.c src/codegen.h src/ast.h src/symbols.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

$(BUILD_DIR)/c_grammar.o: $(BUILD_DIR)/grammar_c.tab.c src/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc -I$(BUILD_DIR) -c $< -o $@

$(BUILD_DIR)/c_lex.o: $(BUILD_DIR)/lex_c.yy.c $(BUILD_DIR)/grammar_c.tab.h src/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc -I$(BUILD_DIR) -c $< -o $@

# Generate parser from grammar
$(BUILD_DIR)/grammar_c.tab.c $(BUILD_DIR)/grammar_c.tab.h: src/grammar.y | $(BUILD_DIR)
	@echo "Generating C port parser..."
	bison -d -o $(BUILD_DIR)/grammar_c.tab.c src/grammar.y

# Generate lexer from specification
$(BUILD_DIR)/lex_c.yy.c: src/lexer.l $(BUILD_DIR)/grammar_c.tab.h | $(BUILD_DIR)
	@echo "Generating C port lexer..."
	flex -o $@ src/lexer.l

# Test targets
test: test-unit test-integration

test-unit: $(BUILD_DIR) $(C_TEST_BINARIES)
	@echo "Running C Port unit tests..."
	@for test in $(C_TEST_BINARIES); do \
		echo "Running $$test..."; \
		$$test; \
	done

test-integration: $(TARGET) | $(TEST_OUTPUT)
	@echo "Running C port integration tests..."
	@echo "====================================================="
	@failed=0; total=0; \
	for test_file in $(TEST_FIXTURES)/*.c; do \
		if [ -f "$$test_file" ]; then \
			total=$$((total + 1)); \
			test_name=$$(basename "$$test_file" .c); \
			printf "Testing $$test_name... "; \
			if $(TARGET) "$$test_file" > /dev/null 2>&1; then \
				echo "PASSED"; \
			else \
				echo "FAILED"; \
				failed=$$((failed + 1)); \
			fi; \
		fi; \
	done; \
	echo "====================================================="; \
	echo "C Port Integration Results: $$((total - failed))/$$total passed"; \
	[ $$failed -eq 0 ]

$(BUILD_DIR)/test_memory_c: tests/unit/test_memory.c src/memory.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc -o $@ $^

$(BUILD_DIR)/test_error_c: tests/unit/test_error.c src/error.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc -o $@ $^

$(BUILD_DIR)/test_ast_c: tests/unit/test_ast_c.c src/ast.c src/symbols.c src/memory.c src/error.c $(BUILD_DIR)/grammar_c.tab.c $(BUILD_DIR)/lex_c.yy.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc -I$(BUILD_DIR) -o $@ $^

$(BUILD_DIR)/test_enum_c: tests/unit/test_enum.c src/ast.c src/symbols.c src/memory.c src/error.c $(BUILD_DIR)/grammar_c.tab.c $(BUILD_DIR)/lex_c.yy.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc -I$(BUILD_DIR) -o $@ $^

$(BUILD_DIR)/test_typedef_c: tests/unit/test_typedef.c src/ast.c src/symbols.c src/memory.c src/error.c $(BUILD_DIR)/grammar_c.tab.c $(BUILD_DIR)/lex_c.yy.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc -I$(BUILD_DIR) -o $@ $^

$(BUILD_DIR)/test_struct_c: tests/unit/test_struct.c src/ast.c src/symbols.c src/memory.c src/error.c $(BUILD_DIR)/grammar_c.tab.c $(BUILD_DIR)/lex_c.yy.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc -I$(BUILD_DIR) -o $@ $^

$(BUILD_DIR)/test_member_access_c: tests/unit/test_member_access.c src/ast.c src/symbols.c src/memory.c src/error.c src/codegen.c $(BUILD_DIR)/grammar_c.tab.c $(BUILD_DIR)/lex_c.yy.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc -I$(BUILD_DIR) -o $@ $^

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean test test-unit test-integration