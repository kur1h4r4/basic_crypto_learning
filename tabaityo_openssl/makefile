CXX := g++
CXXFLAGS := -std=c++20 -O2 -Wall
LDFLAGS := -lssl -lcrypto

TARGET := tabaityo_openssl
SRC := tabaityo_openssl.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)
