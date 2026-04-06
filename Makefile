# GDScript Compiler Makefile

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g
LDFLAGS = 

# Optionally enable static linking of libstdc++/libgcc to reduce runtime DLL
# dependencies on Windows. Set STATIC_LINK=1 when invoking make to enable.
STATIC_LINK ?= 1
ifeq ($(STATIC_LINK),1)
LDFLAGS += -static-libstdc++ -static-libgcc
endif

# Directories
SRCDIR = .
OBJDIR = obj
BINDIR = bin

# Target executable
TARGET = $(BINDIR)/gdscript-compiler

# Source files
SOURCES = main.cpp lexer.cpp parser.cpp semantic_analyzer.cpp code_generator.cpp

# Object files
OBJECTS = $(SOURCES:%.cpp=$(OBJDIR)/%.o)

# Header files
HEADERS = lexer.h parser.h semantic_analyzer.h code_generator.h

# Default target
all: $(TARGET)

# Create directories if they don't exist
$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(BINDIR):
	@mkdir -p $(BINDIR)

# Build target executable
$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build complete: $@"

# Build object files
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(HEADERS) | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@rm -rf $(OBJDIR) $(BINDIR)
	@echo "Clean complete"

# Clean and rebuild
rebuild: clean all

# Install (copy to system path)
install: $(TARGET)
	@echo "Installing GDScript compiler..."
	@sudo cp $(TARGET) /usr/local/bin/
	@echo "Installation complete"

# Uninstall
uninstall:
	@echo "Uninstalling GDScript compiler..."
	@sudo rm -f /usr/local/bin/gdscript-compiler
	@echo "Uninstall complete"

# Run tests
test: $(TARGET)
	@echo "Running tests..."
	@mkdir -p test_output
	@./$(TARGET) examples/hello_world.gd test_output/test
	@echo "Test complete"

# Debug build
debug: CXXFLAGS += -DDEBUG -g3 -O0
debug: $(TARGET)

# Release build
release: CXXFLAGS += -DNDEBUG -O3
release: $(TARGET)

# Profile build
profile: CXXFLAGS += -pg
profile: LDFLAGS += -pg
profile: $(TARGET)

# Static analysis
analyze:
	@echo "Running static analysis..."
	@cppcheck --enable=all --std=c++17 $(SOURCES)

# Format code
format:
	@echo "Formatting code..."
	@clang-format -i $(SOURCES) $(HEADERS)

# Generate documentation
docs:
	@echo "Generating documentation..."
	@doxygen Doxyfile 2>/dev/null || echo "Doxygen not configured"

# Show help
help:
	@echo "GDScript Compiler Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all       - Build the compiler (default)"
	@echo "  clean     - Remove build artifacts"
	@echo "  rebuild   - Clean and build"
	@echo "  install   - Install to system path"
	@echo "  uninstall - Remove from system path"
	@echo "  test      - Run basic tests"
	@echo "  debug     - Build with debug symbols"
	@echo "  release   - Build optimized release"
	@echo "  profile   - Build with profiling support"
	@echo "  analyze   - Run static analysis"
	@echo "  format    - Format source code"
	@echo "  docs      - Generate documentation"
	@echo "  help      - Show this help"
	@echo "  static    - Build with -static-libstdc++ and -static-libgcc (Windows)"
	@echo ""
	@echo "Usage examples:"
	@echo "  make                    # Build compiler"
	@echo "  make clean all          # Clean build"
	@echo "  make debug              # Debug build"
	@echo "  make test               # Run tests"
	@echo "  make install            # Install compiler"

# Phony targets
.PHONY: all clean rebuild install uninstall test debug release profile analyze format docs help

# Dependency tracking
-include $(OBJECTS:.o=.d)

# Generate dependency files
$(OBJDIR)/%.d: $(SRCDIR)/%.cpp | $(OBJDIR)
	@$(CXX) $(CXXFLAGS) -MM -MT $(@:.d=.o) $< > $@

# Print variables (for debugging makefile)
print-%:
	@echo '$*=$($*)'

# Check if required tools are available
check-tools:
	@echo "Checking required tools..."
	@which $(CXX) > /dev/null || (echo "Error: $(CXX) not found" && exit 1)
	@echo "All required tools are available"

# Show compiler version
version:
	@echo "Compiler version:"
	@$(CXX) --version

# Show build configuration
config:
	@echo "Build Configuration:"
	@echo "  CXX:      $(CXX)"
	@echo "  CXXFLAGS: $(CXXFLAGS)"
	@echo "  LDFLAGS:  $(LDFLAGS)"
	@echo "  TARGET:   $(TARGET)"
	@echo "  SOURCES:  $(SOURCES)"
	@echo "  OBJECTS:  $(OBJECTS)"