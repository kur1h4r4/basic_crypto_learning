CXX := g++
CXXFLAGS := -std=c++20 -O2 -Wall
LDFLAGS := -lssl -lcrypto

TARGET := tabaityo_mongomery
SRC := tabaityo_mongomery.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)
