#ifndef RSA_UTILS_HPP
#define RSA_UTILS_HPP

#include "bignum.hpp"
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>

//16進数から2進数への変換(ゴリ押し感が否めないが暫定的に)
inline std::string htob(const std::string& hex) {
    std::string bin;
    for(size_t i=0; i<hex.size(); i++){
        if (hex[i] == '0') bin += "0000";
        else if (hex[i] == '1') bin += "0001";
        else if (hex[i] == '2') bin += "0010";
        else if (hex[i] == '3') bin += "0011";
        else if (hex[i] == '4') bin += "0100";
        else if (hex[i] == '5') bin += "0101";
        else if (hex[i] == '6') bin += "0110";
        else if (hex[i] == '7') bin += "0111";
        else if (hex[i] == '8') bin += "1000";
        else if (hex[i] == '9') bin += "1001";
        else if (hex[i] == 'A' || hex[i] == 'a') bin += "1010";
        else if (hex[i] == 'B' || hex[i] == 'b') bin += "1011";
        else if (hex[i] == 'C' || hex[i] == 'c') bin += "1100";
        else if (hex[i] == 'D' || hex[i] == 'd') bin += "1101";
        else if (hex[i] == 'E' || hex[i] == 'e') bin += "1110";
        else if (hex[i] == 'F' || hex[i] == 'f') bin += "1111";
        else exit(1);
    }
    return bin;
}

// ビット列（0/1 の配列）から代入する
template <size_t N>
void assign_from_bitstring(BigNum<N>& num, const std::string& s) {
    // 最初に0埋め
    num.digits.fill(0);

    // stringを左から順に見ていく
    // リトルエンディアン方式で上位ビットから順に奥の配列に配置
    // stringで1のところにビット立てる
    const size_t bitlen = s.size();
        for (size_t i = 0; i < bitlen; ++i) {
            if (s[i] == '0') {
                continue;
            }

            // 配列の中身がuint64なので64の商と剰余でビットの場所を決める
            size_t bit_index = bitlen - 1 - i;
            size_t word   = bit_index / 64;
            size_t offset = bit_index % 64;

            // 決めたビットの場所に1を立てる
            if (word < N) {
                num.digits[word] |= (uint64_t(1) << offset);
            }
        }
}

// ビット長を返す。与えられたsize分だけ、uint64_tの配列を見ていく。
// ようは、最上位ビットの位置からビット長を算出して返す。
// リトルエンディアンであるため、配列の奥から見ていけば最上位ビットの位置がわかるはず。
inline int bit_length_uint64(const uint64_t* x, size_t len) {
    // __builtin_clzllは引数の頭からの0の数を返す
    // returnするのは、最上位ビットの位置 = ビット長
    for (int i = int(len) - 1; i >= 0; --i) {
        if (x[i] != 0) {
            return i * 64 + 64 - __builtin_clzll(x[i]);
        }
    }
    return 0;
}

// 16進表現（表示用）
template <size_t N>
std::string to_hex(const BigNum<N>& num)
{
    // 文字列結合変数ossを用意
    std::ostringstream oss;

    // uint64_t配列インデックスの中で、最上位の非ゼロ配列を探す
    int msw = N - 1;
    while (msw >= 0 && num.digits[msw] == 0)
        --msw;

    // 全部ゼロなら
    if (msw < 0)
        return "0";

    // ossにはこれから入れる文字列はすべてhexにしてもらう
    oss << std::hex;

    // 最上位ワード（ゼロ埋めしない）
    oss << num.digits[msw];

    // 残りは16桁ゼロ埋め
    for (int i = msw - 1; i >= 0; --i)
    {
        oss << std::setw(16)
            << std::setfill('0')
            << num.digits[i];
    }

    return oss.str();
}

// 多倍長整数を1ビット右シフト
template <size_t N>
void shift_right_one_bit(BigNum<N>& x) {
    uint64_t carry = 0;
    for (int i = N - 1; i >= 0; --i) {
        uint64_t new_carry = x.digits[i] << 63;
        x.digits[i] = (x.digits[i] >> 1) | carry;
        carry = new_carry;
    }
}

#endif