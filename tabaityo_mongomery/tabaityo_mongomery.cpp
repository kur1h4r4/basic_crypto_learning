#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <cstring>
#include <chrono>
#include <iomanip>
#include <sstream>

// --- 基本構造体とユーティリティ ---
template <size_t N>
struct BigNum {
    std::array<uint64_t, N> digits{};
};

// --- 文字列処理・基本判定 ---

std::string htob(const std::string& hex) {
    std::string bin;
    for(size_t i=0; i<hex.size(); i++){
        char c = hex[i];
        int val = (c >= 'a') ? (c - 'a' + 10) : (c >= 'A') ? (c - 'A' + 10) : (c - '0');
        for(int b=3; b>=0; --b) bin += ((val >> b) & 1) ? '1' : '0';
    }
    return bin;
}

template <size_t N>
void assign_from_bitstring(BigNum<N>& num, const std::string& s) {
    num.digits.fill(0);
    const size_t bitlen = s.size();
    for (size_t i = 0; i < bitlen; ++i) {
        if (s[i] == '1') {
            size_t bit_index = bitlen - 1 - i;
            size_t word = bit_index / 64;
            size_t offset = bit_index % 64;
            if (word < N) num.digits[word] |= (uint64_t(1) << offset);
        }
    }
}

int bit_length_uint64(const uint64_t* x, size_t len) {
    for (int i = int(len) - 1; i >= 0; --i) {
        if (x[i] != 0) return i * 64 + 64 - __builtin_clzll(x[i]);
    }
    return 0;
}

template <size_t N>
std::string to_hex(const BigNum<N>& num) {
    std::ostringstream oss;
    int msw = N - 1;
    while (msw >= 0 && num.digits[msw] == 0) --msw;
    if (msw < 0) return "0";
    oss << std::hex << num.digits[msw];
    for (int i = msw - 1; i >= 0; --i) oss << std::setw(16) << std::setfill('0') << num.digits[i];
    return oss.str();
}

template <size_t N> bool is_zero(const BigNum<N>& x) {
    for (auto d : x.digits) if (d != 0) return false;
    return true;
}

template <size_t N> bool is_odd(const BigNum<N>& x) { return (x.digits[0] & 1) != 0; }

template <size_t N>
void shift_right_one_bit(BigNum<N>& x) {
    uint64_t carry = 0;
    for (int i = N - 1; i >= 0; --i) {
        uint64_t next_carry = (x.digits[i] & 1) << 63;
        x.digits[i] = (x.digits[i] >> 1) | carry;
        carry = next_carry;
    }
}

// --- 演算関数 ---

template<size_t N>
uint64_t addT(uint64_t *z, const uint64_t *x, const uint64_t *y) {
    uint64_t c = 0;
    for (size_t i = 0; i < N; i++) {
        __uint128_t v = (__uint128_t)x[i] + y[i] + c;
        z[i] = (uint64_t)v;
        c = (uint64_t)(v >> 64);
    }
    return c;
}

template<size_t N>
uint64_t subT(uint64_t *z, const uint64_t *x, const uint64_t *y) {
    uint64_t borrow = 0;
    for (size_t i = 0; i < N; i++) {
        uint64_t xi = x[i];
        uint64_t yi = y[i];
        uint64_t next_borrow = (xi < yi + borrow) || (yi == 0xFFFFFFFFFFFFFFFFULL && borrow == 1);
        z[i] = xi - yi - borrow;
        borrow = next_borrow;
    }
    return borrow;
}

template <size_t N>
int compare(const uint64_t* a, const uint64_t* b) {
    for (int i = N - 1; i >= 0; --i) {
        if (a[i] > b[i]) return 1;
        if (a[i] < b[i]) return -1;
    }
    return 0;
}

template<size_t N>
void mulT(uint64_t *pz, const uint64_t *px, const uint64_t *py) {
    std::memset(pz, 0, sizeof(uint64_t) * 2 * N);
    for (size_t j = 0; j < N; j++) {
        if (py[j] == 0) continue;
        uint64_t c = 0;
        for (size_t i = 0; i < N; i++) {
            __uint128_t v = (__uint128_t)px[i] * py[j] + pz[i + j] + c;
            pz[i + j] = (uint64_t)v;
            c = (uint64_t)(v >> 64);
        }
        pz[j + N] = c;
    }
}

// 筆算除算 (定数計算や逆元計算に使用)
template <size_t NX, size_t NY>
void divT(uint64_t* q, uint64_t* r, const uint64_t* x, const uint64_t* y) {
    std::memset(q, 0, sizeof(uint64_t) * NX);
    std::memset(r, 0, sizeof(uint64_t) * NY);
    int NA = bit_length_uint64(x, NX);
    int NB = bit_length_uint64(y, NY);
    if (NB == 0) return;
    if (NA < NB) { std::memcpy(r, x, sizeof(uint64_t) * NY); return; }
    for (int i = NA - 1; i >= 0; --i) {
        for (int j = NY - 1; j > 0; --j) r[j] = (r[j] << 1) | (r[j - 1] >> 63);
        r[0] = (r[0] << 1) | ((x[i / 64] >> (i % 64)) & 1);
        if (compare<NY>(r, y) >= 0) {
            subT<NY>(r, r, y);
            q[i / 64] |= (uint64_t(1) << (i % 64));
        }
    }
}

// 逆元 (CRT/モンゴメリ事前準備用)
template <size_t N>
BigNum<N> modinv(BigNum<N> a, const BigNum<N>& m) {
    BigNum<N> b = m, u{}, v{}, q_tmp, r_tmp;
    u.digits[0] = 1;
    while (!is_zero(b)) {
        divT<N, N>(q_tmp.digits.data(), r_tmp.digits.data(), a.digits.data(), b.digits.data());
        uint64_t widebuf[2 * N] = {0};
        mulT<N>(widebuf, q_tmp.digits.data(), v.digits.data());
        BigNum<N> tv, q2;
        divT<2 * N, N>(q2.digits.data(), tv.digits.data(), widebuf, m.digits.data());
        BigNum<N> next_u = (compare<N>(u.digits.data(), tv.digits.data()) < 0) ? u : u;
        if (compare<N>(u.digits.data(), tv.digits.data()) < 0) addT<N>(next_u.digits.data(), u.digits.data(), m.digits.data());
        subT<N>(next_u.digits.data(), next_u.digits.data(), tv.digits.data());
        a = b; b = r_tmp; u = v; v = next_u;
    }
    return u;
}

// --- モンゴメリ演算 ---

uint64_t compute_n_prime(uint64_t n0) {
    uint64_t inv = 1;
    for (int i = 0; i < 6; i++) inv *= 2 - n0 * inv; // Newton法でmod 2^64の逆元
    return -inv;
}

template <size_t N>
void monty_redc(uint64_t* res, uint64_t* T, const uint64_t* n, uint64_t n_prime) {
    for (size_t i = 0; i < N; i++) {
        uint64_t m = T[i] * n_prime;
        uint64_t c = 0;
        for (size_t j = 0; j < N; j++) {
            __uint128_t v = (__uint128_t)m * n[j] + T[i + j] + c;
            T[i + j] = (uint64_t)v;
            c = (uint64_t)(v >> 64);
        }
        uint64_t carry = c;
        for (size_t k = i + N; carry > 0 && k < 2 * N; k++) {
            __uint128_t v = (__uint128_t)T[k] + carry;
            T[k] = (uint64_t)v;
            carry = (uint64_t)(v >> 64);
        }
    }
    std::memcpy(res, T + N, sizeof(uint64_t) * N);
    if (compare<N>(res, n) >= 0) subT<N>(res, res, n);
}
// モンゴメリ形式での乗算 (res = a * b * R^-1 mod n)
template <size_t N>
void monty_mul(uint64_t* res, const uint64_t* a, const uint64_t* b, const uint64_t* n, uint64_t n_prime) {
    uint64_t T[2 * N] = {0}; // 毎回ゼロクリアが必要
    mulT<N>(T, a, b);
    monty_redc<N>(res, T, n, n_prime);
}

template <size_t N>
BigNum<N> modexp_monty(BigNum<N> a, BigNum<N> b, const BigNum<N>& m) {
    if (is_zero(m)) return {};
    
    uint64_t n_prime = compute_n_prime(m.digits[0]);
    
    // R^2 mod m の計算
    BigNum<N> r2, q_dummy;
    uint64_t wide_r[2 * N + 1] = {0};
    wide_r[2 * N] = 1; 
    divT<2 * N + 1, N>(q_dummy.digits.data(), r2.digits.data(), wide_r, m.digits.data());

    // 初期変換
    BigNum<N> a_bar, res_bar, one_bn{};
    one_bn.digits[0] = 1;

    // a_bar = a * R mod m
    monty_mul<N>(a_bar.digits.data(), a.digits.data(), r2.digits.data(), m.digits.data(), n_prime);
    
    // res_bar = 1 * R mod m (モンゴメリ空間における '1')
    // ここが 0 だと全て 0 になります。
    monty_mul<N>(res_bar.digits.data(), one_bn.digits.data(), r2.digits.data(), m.digits.data(), n_prime);

    // バイナリ法 (Montgomery Ladder)
    while (!is_zero(b)) {
        if (is_odd(b)) {
            monty_mul<N>(res_bar.digits.data(), res_bar.digits.data(), a_bar.digits.data(), m.digits.data(), n_prime);
        }
        monty_mul<N>(a_bar.digits.data(), a_bar.digits.data(), a_bar.digits.data(), m.digits.data(), n_prime);
        shift_right_one_bit<N>(b);
    }

    // 通常空間へ戻す: res = res_bar * 1 * R^-1 mod m
    BigNum<N> res;
    monty_mul<N>(res.digits.data(), res_bar.digits.data(), one_bn.digits.data(), m.digits.data(), n_prime);
    
    return res;
}

// --- メイン ---

int main() {
    auto total_start = std::chrono::high_resolution_clock::now();
    constexpr size_t BASE_N = 64; 

    std::string p_hex = "efebb1728416286c7f6106c64809881f5e099bf39a2b6e1ab265cb08756e8d4ea164e109638ff282f63f0d22289e2fb11aee07dcc241a4dba4e296b177ddabd9b53ab541decd495f5a10a2157880689139ce735affeaac076cf390d45a61cf72efc2a7bab9aabd8d40ac8f5020a4d5e7d5ea304e46cb68ba2ac3e6ae28033535";
    std::string q_hex = "d0d1e47cca884cb9aeb9718b7505d50fa5b2f6a6612a209af367f9dfcf13a0a8cbce19f6f96859992631a36ac88ea3b4828be2112413e78651b775570b6ade962a3ee7d1cb4d32bcd4b8ffdd98cd2fef6d3f24c770ac3642d9006c31e305fdffbde8e33033a5919f264c27fcb645430e130c833b1504593da8967a6bbdf09c3f";
    
    BigNum<BASE_N> p, q, e, m, one{}, pminus, qminus, d, toshent;
    assign_from_bitstring(p, htob(p_hex));
    assign_from_bitstring(q, htob(q_hex));
    assign_from_bitstring(e, htob("10001"));
    assign_from_bitstring(m, htob("ff"));
    one.digits[0] = 1;

    subT<BASE_N>(pminus.digits.data(), p.digits.data(), one.digits.data());
    subT<BASE_N>(qminus.digits.data(), q.digits.data(), one.digits.data());

    uint64_t wide[2 * BASE_N];
    mulT<BASE_N>(wide, p.digits.data(), q.digits.data());
    BigNum<BASE_N> n; std::memcpy(n.digits.data(), wide, sizeof(uint64_t) * BASE_N);

    mulT<BASE_N>(wide, pminus.digits.data(), qminus.digits.data());
    std::memcpy(toshent.digits.data(), wide, sizeof(uint64_t) * BASE_N);

    // 暗号化
    BigNum<BASE_N> c = modexp_monty(m, e, n);
    std::cout << "Ciphertext c: " << to_hex(c) << std::endl;

    // 復号準備
    d = modinv(e, toshent);
    BigNum<BASE_N> dp, dq, q_d, q1;
    divT<BASE_N, BASE_N>(q_d.digits.data(), dp.digits.data(), d.digits.data(), pminus.digits.data());
    divT<BASE_N, BASE_N>(q_d.digits.data(), dq.digits.data(), d.digits.data(), qminus.digits.data());
    q1 = modinv(q, p);

    // CRT復号
    BigNum<BASE_N> mp = modexp_monty(c, dp, p);
    BigNum<BASE_N> mq = modexp_monty(c, dq, q);

    BigNum<BASE_N> h_diff;
    if (compare<BASE_N>(mp.digits.data(), mq.digits.data()) < 0) {
        addT<BASE_N>(h_diff.digits.data(), mp.digits.data(), p.digits.data());
        subT<BASE_N>(h_diff.digits.data(), h_diff.digits.data(), mq.digits.data());
    } else {
        subT<BASE_N>(h_diff.digits.data(), mp.digits.data(), mq.digits.data());
    }

    uint64_t wide_h[2 * BASE_N];
    mulT<BASE_N>(wide_h, h_diff.digits.data(), q1.digits.data());
    BigNum<BASE_N> h, dummy_h;
    divT<2 * BASE_N, BASE_N>(dummy_h.digits.data(), h.digits.data(), wide_h, p.digits.data());

    mulT<BASE_N>(wide, h.digits.data(), q.digits.data());
    BigNum<BASE_N> final_m; std::memcpy(final_m.digits.data(), wide, sizeof(uint64_t) * BASE_N);
    addT<BASE_N>(final_m.digits.data(), final_m.digits.data(), mq.digits.data());

    std::cout << "Decrypted m: " << to_hex(final_m) << std::endl;

    auto total_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = total_end - total_start;
    std::cout << "Total time (Montgomery + CRT): " << diff.count() << " sec" << std::endl;

    return 0;
}