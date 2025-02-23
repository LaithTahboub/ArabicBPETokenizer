# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++2a -Wall -Wextra -pedantic

# Executable name
TARGET = BPE

# Source files
SOURCES = BPE.cpp

# Object files (automatically derived from sources)
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Linking the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

# Compile source files into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(TARGET) $(OBJECTS)

.PHONY: all clean
