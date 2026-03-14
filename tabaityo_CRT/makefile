CXX      := g++
CXXFLAGS := -std=c++20 -O2 -Wall -Wextra -pedantic
TARGET   := tabaityo_CRT
SRC      := main.cpp
# 依存するすべてのヘッダーファイルをリストアップ
HEADERS  := bignum.hpp rsa_utils.hpp rsa_ops.hpp

all: $(TARGET)

# $^ は依存ファイル($(SRC) $(HEADERS))、$@ はターゲット名($(TARGET))を指します
$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean