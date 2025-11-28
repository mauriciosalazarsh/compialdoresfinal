CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -Iinclude
LDFLAGS =

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
TEST_DIR = tests
EXAMPLE_DIR = examples

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Target executable
TARGET = compiler

.PHONY: all clean test examples run

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR) $(TARGET) *.s *.o program
	rm -f $(TEST_DIR)/*.s $(TEST_DIR)/program
	rm -f $(EXAMPLE_DIR)/*.s $(EXAMPLE_DIR)/program

test: $(TARGET)
	@echo "Running test cases..."
	@for test in $(TEST_DIR)/*.kt; do \
		echo "\n=== Testing $$test ==="; \
		./$(TARGET) $$test -o $${test%.kt}.s && \
		gcc -no-pie $${test%.kt}.s -o $${test%.kt}.out && \
		$${test%.kt}.out; \
	done

examples: $(TARGET)
	@echo "Compiling examples..."
	@for example in $(EXAMPLE_DIR)/*.kt; do \
		echo "\n=== Compiling $$example ==="; \
		./$(TARGET) $$example -o $${example%.kt}.s; \
	done

run: $(TARGET)
	@if [ -z "$(FILE)" ]; then \
		echo "Usage: make run FILE=<filename.kt>"; \
	else \
		./$(TARGET) $(FILE) -o output.s && \
		gcc -no-pie output.s -o program && \
		./program; \
	fi

help:
	@echo "Kotlin Compiler - Makefile Commands"
	@echo "===================================="
	@echo "make          - Build the compiler"
	@echo "make clean    - Remove build files"
	@echo "make test     - Run all test cases"
	@echo "make examples - Compile all examples"
	@echo "make run FILE=<file.kt> - Compile and run a specific file"
	@echo ""
	@echo "Manual usage:"
	@echo "  ./compiler input.kt -o output.s"
	@echo "  gcc -no-pie output.s -o program"
	@echo "  ./program"
