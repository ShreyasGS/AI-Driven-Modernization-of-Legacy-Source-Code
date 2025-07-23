CXX = g++
CXXFLAGS = -std=c++11 -Wall -I. -I../../ -DMOZILLA_INTERNAL_API

# Define the target
TARGET = test_modernized_selection_simple

# Define the source files
SOURCES = test_modernized_selection_simple.cpp modernized_methods.cpp mock_nsTypedSelection.cpp

# Define the object files
OBJECTS = $(SOURCES:.cpp=.o)

# Define the header files
HEADERS = mock_nsTypedSelection.h modernized_methods.h modernized_nsSelection_Result.h nsIDOMNode.h nsIDOMRange.h nsIPresContext.h nsCOMPtr.h nsIArray.h nsAString.h

# Default target
all: $(TARGET)

# Link the target
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile the source files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Run the tests
run: $(TARGET)
	./$(TARGET)

# Clean up
clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all run clean 