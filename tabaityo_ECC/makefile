CXX      := g++
CXXFLAGS := -std=c++20 -O2 -Wall -Wextra -pedantic
TARGET   := tabaityo_ecdsa_sign_verify

# ソースファイルとヘッダーファイルの定義
SRC      := main.cpp
HEADERS  := bignum.hpp rsa_utils.hpp ecdsa_ops.hpp

all: $(TARGET)

# TARGETはSRCとすべてのHEADERSに依存する
$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean