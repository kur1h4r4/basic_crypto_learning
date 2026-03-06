CXX := g++
CXXFLAGS := -std=c++20 -O2 -Wall -Wextra -pedantic
TARGET := tabaityo_simple
SRC := tabaityo_simple.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

.PHONY: all clean
