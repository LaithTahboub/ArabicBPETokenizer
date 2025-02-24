# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic
LDFLAGS = -lstdc++fs  # Moved to linker flags

# Executable name
TARGET = BPE

# Source files
SOURCES = BPE.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

# Link with LDFLAGS after objects
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)  

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS)

.PHONY: all clean