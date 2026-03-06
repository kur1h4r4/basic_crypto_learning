CXX := g++
CXXFLAGS := -std=c++20 -O2 -Wall -Wextra -pedantic
TARGET := tabaityo_CRT
SRC := tabaityo_CRT.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

.PHONY: all clean
