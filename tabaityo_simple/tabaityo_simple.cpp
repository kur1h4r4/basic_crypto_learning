#include <random>
#include <iostream>
#include <cstring>
#include <random>
#include <chrono>
#include <vector>
#include <array>
#include <sstream>
#include <iomanip>
#include <string>
#include <cstdint>
#include <bitset>

//16進数から2進数への変換(ゴリ押し感が否めないが暫定的に)
std::string htob(const std::string& hex) {
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

// 多倍長整数構造体
template <size_t N>
struct BigNum {
    std::array<uint64_t, N> digits{}; 
};


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
int bit_length_uint64(const uint64_t* x, size_t len) {
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
    uint64_t c = 0;
    for (size_t i = 0; i < N; i++) {
        uint64_t xc = x[i] - c;
        c = xc > x[i];

        uint64_t yi = y[i];
        xc -= yi;
        c += xc > x[i] - c;
        z[i] = xc;
    }
    return c;
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



// 左シフト(割り算の割られる数(x)を上位から見ていくときに使う。実際は、余りを左シフトしていくということ。)
template <size_t N>
void shift_left_one_bit_uint64(uint64_t* num) {
    // リトルエンディアンなので、インデックスが大きいほうが上位ビット。
    for (int i = N - 1; i > 0; --i) {
        // 上位ビットを1個左シフトする。その際、下位ビット配列の先頭ビットを上位ビット配列の末尾ビットに入れる。
        // これを多倍長整数の配列インデックスの数だけ繰り返す。
        num[i] = (num[i] << 1) | (num[i - 1] >> 63);
    }
    // 最期に最上位ビットも左シフト。
    num[0] <<= 1;
}

// 大小比較関数
template <size_t N>
int compare(const uint64_t* a, const uint64_t* b){
    // ビット長から桁数算出
    int len_a = bit_length_uint64(a, N);
    int len_b = bit_length_uint64(b, N);

    if (len_a != len_b){
        return (len_a > len_b) ? 1 : -1;
    }

    // 桁数同じなら、上から見てって先に1が出てきたほうが大きい
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


    // xとyのビット長を先に算出。
    int NA = bit_length_uint64(x, NX);
    int NB = bit_length_uint64(y, NY);

    // 0で割ることはできない。
    if (NB == 0) {
        return;
    }

    // 割る数ビット長が長い、つまり割る数のほうが大きいときは、余りがそのまま割られる数に。
    if (NA < NB) {
        std::memcpy(r, x, sizeof(uint64_t) * NY);
        return;
    }

    // A ÷ B を筆算で求める
    // 割られる数の桁数分ループ
    // iは多倍長整数の配列インデックスのビットの場所を指す。ようは、割られる数の上の桁から見ていきたいのでってこと。
    for (int i = NA - 1; i >= 0; --i) {
        // 余りを左シフトし、最上位ビットには0が入る
        shift_left_one_bit_uint64<NY>(r);

        // wordはiで今見ている多倍長整数の配列インデックス(64ビット単位のインデックス)。
        // bitはiで今見ている配列の中のビット位置。(64ビット単位のインデックスの中でどのビットを走査中か。)
        // r[0] OR x[i](割られる数の走査中のビット位置)。つまり、x[i]が1なら1になる。
        int word = i / 64;
        int bit  = i % 64;
        r[0] |= (x[word] >> bit) & 1;

        // もしr(あまり)がy(割られる数)より大きいなら、r = r - y
        if (compare<NY>(r, y) >= 0) {

            // r -= y
            subT<NY>(r, r, y);

            // 多倍長整数全体の、i番目の商に1を立てる。(商の桁数はNA-1桁だから。)
            q[word] |= (uint64_t(1) << bit);
        }
    }
}

// 逆元
template <size_t N>
BigNum<N> modinv(BigNum<N> a, const BigNum<N>& m)
{
    BigNum<N> b = m;

    // u = 1, v = 0
    BigNum<N> u{}, v{};
    u.digits[0] = 1;

    while (true)
    {
        // b == 0 ?
        if(is_zero<N>(b)){
            break;
        }

        BigNum<N> t{}, r{};

        // a = t*b + r
        divT<N, N>(t.digits.data(), r.digits.data(), a.digits.data(), b.digits.data());

        // tv = (t * v) % m
        // tはN桁、vはN桁なので、2N桁入るmulbufを用意し、それをmod mしてN桁未満に戻す。
        uint64_t mulbuf[2*N] = {};
        mulT<N>(mulbuf, t.digits.data(), v.digits.data());

        BigNum<N> qtmp{}, tv{};
        divT<2*N, N>(qtmp.digits.data(), tv.digits.data(), mulbuf, m.digits.data());

        // unsigned_u = (u - tv) mod m
        // tvを引く前に、uが負ならmを足してmod mの範囲の正にしていく。
        BigNum<N> unsigned_u;

        if (compare<N>(u.digits.data(), tv.digits.data()) < 0)
        {
            // u + m
            addT<N>(unsigned_u.digits.data(), u.digits.data(), m.digits.data());
        }
        else
        {
            unsigned_u = u;
        }

        // unsigned_u -= tv
        subT<N>(unsigned_u.digits.data(), unsigned_u.digits.data(), tv.digits.data());

        // ループごとに状態更新していく
        a = b;
        b = r;

        u = v;
        v = unsigned_u;
    }

    // u % m を返す
    BigNum<N> q{}, r{};
    divT<N, N>(q.digits.data(), r.digits.data(), u.digits.data(), m.digits.data());

    return r;
}



template <size_t N>
BigNum<N> modexp(BigNum<N> a, BigNum<N> b, const BigNum<N>& m)
{
    BigNum<N> result{};
    result.digits[0] = 1;   // result = 1

    while (!is_zero(b)){
        // バイナリ法
        // bを下位ビットから見ていき(bは毎ループ右シフト)、奇数ならaを一回かける。偶数ならa^2
        // つまり、指数が奇数なら a + 1、偶数ならa^2ずつ増えるということ。
        // b = 1101(13) 指数⇒a + a^2^2(a^4) + a^2^2^2(a^8) (1 + 4 + 8 = 13)
        if (is_odd(b)){
            // result = (result * a) % m

            uint64_t mulbuf[2*N] = {};
            mulT<N>(mulbuf, result.digits.data(), a.digits.data());

            BigNum<N> qtmp{};
            BigNum<N> rtmp{};

            divT<2*N, N>(qtmp.digits.data(), rtmp.digits.data(), mulbuf, m.digits.data());

            result = rtmp;
        }

        // a = (a * a) % m
        {
            uint64_t mulbuf[2*N] = {};
            mulT<N>(mulbuf, a.digits.data(), a.digits.data());

            BigNum<N> qtmp{}, rtmp{};
            divT<2*N, N>(qtmp.digits.data(), rtmp.digits.data(), mulbuf, m.digits.data());

            a = rtmp;
        }

        // b >>= 1
        shift_right_one_bit<N>(b);
    }

    return result;
}


int main(){
    auto total_start = std::chrono::high_resolution_clock::now();
    // Nはbit数の2倍確保しておく。mulTやdivTでは2N桁になる場合があるため。2048 /64 = 32の2倍 = 64　 
    constexpr size_t BASE_N = 64;
    BigNum<BASE_N> p, q, n, d, e, m, c, pminus, qminus, toshent, decrypt_m, one;
    std::string pbits, qbits, ebits, mbits;

    // 256bit
    //pbits = htob("f450a212027568630566c6e3866c8af0a21a0fa397bf2840386a59416a821f15");
    //qbits = htob("c0abede688f21e730963a79a37179405268b2b838b365d9bda83808dadac59ef");
    
    // 1024bit
    //pbits = htob("e99a618870b73b47b3b11aba33783fd36a237a9e1afbe00dbccc3586217078ed08d15a2f51a5f7d3f77a05c0549e773ce2a7f713c5f6bc4a8bbb97f07ea1bcbf");
    //qbits = htob("dc68ec7a3c38a98c9fb987f3f7bac2a58fc703eba09abcf18eb34c82f81f0444afffb78fc98a030cbe6d05ac037bffeccfdc49113b214ad016e3908317c49451");
    
    // 2048bit
    pbits = htob("efebb1728416286c7f6106c64809881f5e099bf39a2b6e1ab265cb08756e8d4ea164e109638ff282f63f0d22289e2fb11aee07dcc241a4dba4e296b177ddabd9b53ab541decd495f5a10a2157880689139ce735affeaac076cf390d45a61cf72efc2a7bab9aabd8d40ac8f5020a4d5e7d5ea304e46cb68ba2ac3e6ae28033535");
    qbits = htob("d0d1e47cca884cb9aeb9718b7505d50fa5b2f6a6612a209af367f9dfcf13a0a8cbce19f6f96859992631a36ac88ea3b4828be2112413e78651b775570b6ade962a3ee7d1cb4d32bcd4b8ffdd98cd2fef6d3f24c770ac3642d9006c31e305fdffbde8e33033a5919f264c27fcb645430e130c833b1504593da8967a6bbdf09c3f");
    ebits = htob("10001"); //65537
    mbits = htob("ff");

    // 10進数文字列から代入
    assign_from_bitstring(p, pbits);
    assign_from_bitstring(q, qbits);
    assign_from_bitstring(e, ebits);
    assign_from_bitstring(m, mbits);
    assign_from_bitstring(one, "1");

    std::cout << "p (hex) = " << to_hex(p) << std::endl;
    std::cout << "q (hex) = " << to_hex(q) << std::endl;
    std::cout << "e (hex) = " << to_hex(e) << std::endl;

    // Nの算出
    // mulTは乗算結果が2N桁になるので、バッファを2N桁確保しておく。
    uint64_t widebuf[BASE_N * 2] = {};
    mulT<BASE_N>(widebuf, p.digits.data(), q.digits.data());

    std::memcpy(n.digits.data(), widebuf, sizeof(uint64_t) * BASE_N);
    std::memset(widebuf, 0, sizeof(widebuf));

    std::cout << "n (hex) = " << to_hex(n) << std::endl;
    
    // トーシェント関数φを算出
    subT<BASE_N>(pminus.digits.data(), p.digits.data(), one.digits.data());
    subT<BASE_N>(qminus.digits.data(), q.digits.data(), one.digits.data());
    
    mulT<BASE_N>(widebuf, pminus.digits.data(), qminus.digits.data());
    std::memcpy(toshent.digits.data(), widebuf, sizeof(uint64_t) * BASE_N);
    std::memset(widebuf, 0, sizeof(widebuf));

    // 暗号文Cの算出
    c = modexp(m, e, n);
    std::cout << "c (hex) = " << to_hex(c) << std::endl;

    // 秘密鍵Dの算出
    d = modinv(e, toshent);
    std::cout << "d (hex) = " << to_hex(d) << std::endl;
    
    // 平文Mの算出
    decrypt_m = modexp(c, d, n);
    std::cout << "decrypt_m (hex) = " << to_hex(decrypt_m) << std::endl;

    // 計測終了
    auto total_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = total_end - total_start;
    std::cout << "Total time: " << diff.count() << " sec\n";
}

