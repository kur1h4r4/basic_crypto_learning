CXX := g++
CXXFLAGS := -std=c++20 -O2 -Wall -Wextra -pedantic
TARGET := tabaityo_ecc
SRC := tabaityo_ecc.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

.PHONY: all clean
