#ifndef BIGNUM_HPP
#define BIGNUM_HPP

#include <array>
#include <cstdint>
#include <cstring>
#include <string>

// 多倍長整数構造体
template <size_t N>
struct BigNum {
    std::array<uint64_t, N> digits{}; 
};

// ビット長を返す (実体は utils.cpp)
int bit_length_uint64(const uint64_t* x, size_t len);

// 多倍長整数が0かどうかの判定
template <size_t N>
bool is_zero(const BigNum<N>& x) {
    for (size_t i = 0; i < N; ++i)
        if (x.digits[i] != 0) {
            return false;
        }
    return true;
}

// 多倍長整数が奇数かどうかの判定
template <size_t N>
bool is_odd(const BigNum<N>& x) {
    return (x.digits[0] & 1ULL) != 0;
}

// 加算
template<size_t N>
uint64_t addT(uint64_t *z, const uint64_t *x, const uint64_t *y)
{
    uint64_t c = 0;
    for (size_t i = 0; i < N; i++) {
        uint64_t xc = x[i] + c;
        c = xc < c;

        uint64_t yi = y[i];
        xc += yi;
        c += xc < yi;

        z[i] = xc;
    }
    return c;
}

// 減算
template<size_t N>
uint64_t subT(uint64_t *z, const uint64_t *x, const uint64_t *y)
{
    uint64_t borrow = 0;
    for (size_t i = 0; i < N; i++) {
        __uint128_t xi = (__uint128_t)x[i];
        __uint128_t yi = (__uint128_t)y[i];
        __uint128_t tmp = xi - yi - borrow;
        z[i] = (uint64_t)tmp;
        borrow = (tmp >> 127) & 1;
    }
    return borrow;
}

// 乗算 (uint64_t)
template<size_t N>
uint64_t mulUnitT(uint64_t *z, const uint64_t *x, uint64_t y){
    uint64_t H = 0;
    for (size_t i = 0; i < N; i++) {
        __uint128_t v = __uint128_t(x[i]) * y;
        v += H;
        z[i] = uint64_t(v);
        H = uint64_t(v >> 64);
    }
    return uint64_t(H); // z[n]
}

// z[2N] = x[N] * y[N]
template<size_t N>
void mulT(uint64_t *pz, const uint64_t *px, const uint64_t *py)
{
  pz[N] = mulUnitT<N>(pz, px, py[0]); // px[] * py[0]
  for (size_t i = 1; i < N; i++) {
    uint64_t xy[N], ret;
    ret = mulUnitT<N>(xy, px, py[i]); // px[] * py[i]
    pz[N + i] = ret + addT<N>(&pz[i], &pz[i], xy);
  }
}

// 左シフト(割り算の割られる数(x)を上位から見ていくときに使う)
template <size_t N>
void shift_left_one_bit_uint64(uint64_t* num) {
    for (int i = N - 1; i > 0; --i) {
        num[i] = (num[i] << 1) | (num[i - 1] >> 63);
    }
    num[0] <<= 1;
}

// 大小比較関数
template <size_t N>
int compare(const uint64_t* a, const uint64_t* b){
    int len_a = bit_length_uint64(a, N);
    int len_b = bit_length_uint64(b, N);

    if (len_a != len_b){
        return (len_a > len_b) ? 1 : -1;
    }

    for (int i = N - 1; i >= 0; --i) {
        if(a[i] > b[i]) return 1;
        if(a[i] < b[i]) return -1;
    }
    return 0;
}

// 除算 (uint64_t)
template <size_t NX, size_t NY>
void divT(uint64_t* q, uint64_t* r, const uint64_t* x, const uint64_t* y) {
    std::memset(q, 0, sizeof(uint64_t) * NX);
    std::memset(r, 0, sizeof(uint64_t) * NY);

    int NA = bit_length_uint64(x, NX);
    int NB = bit_length_uint64(y, NY);

    if (NB == 0) return;

    if (NA < NB) {
        std::memcpy(r, x, sizeof(uint64_t) * NY);
        return;
    }

    for (int i = NA - 1; i >= 0; --i) {
        shift_left_one_bit_uint64<NY>(r);

        int word = i / 64;
        int bit  = i % 64;
        r[0] |= (x[word] >> bit) & 1;

        if (compare<NY>(r, y) >= 0) {
            subT<NY>(r, r, y);
            q[word] |= (uint64_t(1) << bit);
        }
    }
}

#endif