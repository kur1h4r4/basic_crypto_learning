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
// 被除算引数の桁数によって、template<○,○>を変える必要がある。
// 被除算引数が2N桁なら、<2*N, N>、N桁未満なら<N, N>としなければならない。
// 商の確保サイズも被除算引数と合わせる必要がある。
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

        // a = t*b + r(aはN桁未満なので商tもサイズN確保)
        divT<N, N>(t.digits.data(), r.digits.data(), a.digits.data(), b.digits.data());

        // tv = (t * v) % m
        // tはN桁、vはN桁なので、2N桁入るmulbufを用意し、それをmod mしてN桁未満に戻す。
        uint64_t mulbuf[2*N] = {};
        mulT<N>(mulbuf, t.digits.data(), v.digits.data());

        BigNum<2 * N> qtmp{};
        BigNum<N> tv{};
        // mulbufが2N桁なので、qtmpも2Nサイズ確保。
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
    BigNum<N> q{};
    BigNum<N> r{};
    // uはN桁なので、qもNサイズ確保。
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

            BigNum<2*N> qtmp{};
            BigNum<N> rtmp{};

            divT<2*N, N>(qtmp.digits.data(), rtmp.digits.data(), mulbuf, m.digits.data());

            result = rtmp;
        }

        // a = (a * a) % m
        {
            uint64_t mulbuf[2*N] = {};
            mulT<N>(mulbuf, a.digits.data(), a.digits.data());

            BigNum<2*N> qtmp{};
            BigNum<N> rtmp{};
            divT<2*N, N>(qtmp.digits.data(), rtmp.digits.data(), mulbuf, m.digits.data());

            a = rtmp;
        }

        // b >>= 1
        shift_right_one_bit<N>(b);
    }

    return result;
}

// 剰余加算: (a + b) % m
template <size_t N>
BigNum<N> mod_add(const BigNum<N>& a, const BigNum<N>& b, const BigNum<N>& m) {
    BigNum<N> res;
    // res = a + b
    uint64_t carry = addT<N>(res.digits.data(), a.digits.data(), b.digits.data());
    // carryがある、もしくはresがp以上なら、res = res - p
    if (carry || compare<N>(res.digits.data(), m.digits.data()) >= 0) {
        subT<N>(res.digits.data(), res.digits.data(), m.digits.data());
    }
    return res;
}

// 剰余減算: (a - b) % m
template <size_t N>
BigNum<N> mod_sub(const BigNum<N>& a, const BigNum<N>& b, const BigNum<N>& m){
    BigNum<N> res;
    // a > bならa - b、 a < bならa + m - bをして負数にならないように
    if (compare<N>(a.digits.data(), b.digits.data()) >= 0) {
        subT<N>(res.digits.data(), a.digits.data(), b.digits.data());
    } else {
        // res = a + m - b
        BigNum<N> tmp;
        addT<N>(tmp.digits.data(), a.digits.data(), m.digits.data());
        subT<N>(res.digits.data(), tmp.digits.data(), b.digits.data());
    }

    return res;
}

// 剰余乗算: (a * b) % m
template <size_t N>
BigNum<N> mod_mul(const BigNum<N>& a, const BigNum<N>& b, const BigNum<N>& m) {
    uint64_t mulbuf[2 * N] = {};
    mulT<N>(mulbuf, a.digits.data(), b.digits.data());

    BigNum<2 * N> q{};
    BigNum<N> r;
    divT<2 * N, N>(q.digits.data(), r.digits.data(), mulbuf, m.digits.data());
    return r;
}


// 楕円曲線上の点
// 無限遠点はフラグで管理
template <size_t N>
struct ECPoint {
    BigNum<N> x, y;
    bool is_infinity = false;
};

// 点加算 (P + Q)
template <size_t N>
ECPoint<N> ec_add(const ECPoint<N>& P, const ECPoint<N>& Q, const BigNum<N>& a, const BigNum<N>& m) {
    // 無限遠点は単位元となる。
    // P、Qのどちらかが無限遠点ならば、単位元を足すので、PかQをそのまま返す
    if (P.is_infinity){
        return Q;
    }
    if (Q.is_infinity){
        return P;
    }

    // x座標が同じ場合
    // 楕円曲線の場合、x座標が同じなら、y座標は同一 or y=0対称となるかのどちらかしかない
    if (compare<N>(P.x.digits.data(), Q.x.digits.data()) == 0) {
        // PとQがy=0で対称（P = -Q）なら加算結果は無限遠点となる。
        // -Qのy座標 = m - Qy mod m
        BigNum<N> neg_y = mod_sub(m, Q.y, m);
        if (compare<N>(P.y.digits.data(), neg_y.digits.data()) == 0 || is_zero(P.y)) {
            return {BigNum<N>(), BigNum<N>(), true};
        }

        // P + P(点倍算)
        // lambda = (3x^2 + a) / 2y
        BigNum<N> three{}, two{};
        three.digits[0] = 3; two.digits[0] = 2;

        BigNum<N> num = mod_add(mod_mul(mod_mul(P.x, P.x, m), three, m), a, m);
        BigNum<N> den = modinv(mod_mul(P.y, two, m), m);
        BigNum<N> lambda = mod_mul(num, den, m);

        // x3 = lambda^2 - x1 - x2
        // y3 = lambda * (x1 - x3) - y1
        BigNum<N> x3 = mod_sub(mod_sub(mod_mul(lambda, lambda, m), P.x, m), P.x, m);
        BigNum<N> y3 = mod_sub(mod_mul(lambda, mod_sub(P.x, x3, m), m), P.y, m);
        return {x3, y3, false};
    } else {
        // 通常の加算
        // lambda = (y2 - y1) / (x2 - x1)
        BigNum<N> num = mod_sub(Q.y, P.y, m);
        BigNum<N> den = modinv(mod_sub(Q.x, P.x, m), m);
        BigNum<N> lambda = mod_mul(num, den, m);

        // x3 = lambda^2 - x1 - x2
        // y3 = lambda * (x1 - x3) - y1
        BigNum<N> x3 = mod_sub(mod_sub(mod_mul(lambda, lambda, m), P.x, m), Q.x, m);
        BigNum<N> y3 = mod_sub(mod_mul(lambda, mod_sub(P.x, x3, m), m), P.y, m);
        return {x3, y3, false};
    }
}

// スカラー倍 (k * P)
// バイナリ法で実装
template <size_t N>
ECPoint<N> ec_mul(BigNum<N> k, ECPoint<N> P, const BigNum<N>& a, const BigNum<N>& p) {
    // 初期化は無限遠点
    ECPoint<N> result = {BigNum<N>(), BigNum<N>(), true};
    ECPoint<N> base = P;

    while (!is_zero(k)) {
        if (is_odd(k)) {
            result = ec_add(result, base, a, p);
        }
        base = ec_add(base, base, a, p);
        shift_right_one_bit(k);
    }
    return result;
}

// 署名構造体
template <size_t N>
struct ECDSASignature {
    BigNum<N> r, s;
};

template <size_t N>
BigNum<N> generate_k(const BigNum<N>& n) {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;

    // 1. nが何ワード（uint64_tいくつ分）あるか計算する
    // bit_length / 64 を切り上げれば、有効な配列サイズがわかる
    int n_bits = bit_length_uint64(n.digits.data(), N);
    size_t active_words = (n_bits + 63) / 64; 

    BigNum<N> k;
    while (true) {
        k.digits.fill(0); // 常に 0 で初期化

        // 2. nのワード数分だけランダムに埋める
        for (size_t i = 0; i < active_words; ++i) {
            k.digits[i] = dist(gen);
        }

        // 3. 0 < k < n かチェック
        bool all_zero = true;
        for (size_t i = 0; i < active_words; ++i) if (k.digits[i] != 0) all_zero = false;
        if (all_zero) continue;

        if (compare<N>(k.digits.data(), n.digits.data()) < 0) {
            break; 
        }
    }
    return k;
}

// 署名生成(SEC1参照https://www.secg.org/sec1-v2.pdf)
template <size_t N>
ECDSASignature<N> ecdsa_sign(const BigNum<N>& msg_hash, const BigNum<N>& private_key, const BigNum<N>& n, const BigNum<N>& a, const BigNum<N>& m, const ECPoint<N>& G) {
    // 一時的な秘密鍵k、公開鍵Rを生成
    // 0 < k < nとなるような乱数を生成(範囲で使うのは法mではなく、位数n。)
    BigNum<N> k;
    k = generate_k(n);

    // R = k * G
    ECPoint<N> R = ec_mul(k, G, a, m);
    
    // 署名の一片r = R.x % n
    // mod_mulを使って n で割った余りを得る
    BigNum<N> r = mod_mul(R.x, BigNum<N>{{1}}, n);
    
    // r = 0なら再試行
    if (is_zero(r)){
        return ecdsa_sign(msg_hash, private_key, n, a, m, G);
    }

    // s = k^-1 * (msg_hash + r * private_key) % n
    // 法mではなく、位数nで剰余をとる。
    BigNum<N> k_inv = modinv(k, n);
    BigNum<N> r_d = mod_mul(r, private_key, n);
    BigNum<N> e_rd = mod_add(msg_hash, r_d, n);
    BigNum<N> s = mod_mul(k_inv, e_rd, n);

    // s = 0なら再試行
    if (is_zero(s)){
        return ecdsa_sign(msg_hash, private_key, n, a, m, G);
    }

    return {r, s};
}

// 署名検証
// 署名 s = k^-1 * (msg_hash + r * private_key)
// sk = msg_hash + r * private_key
// k = s^-1 *msg_hash + s^-1 * r + private_key
// k = u1 + u2 * private_key
// kG = u1G + u2G * private_key
// kG = u1G + u2Q
// となるため、u1G + u2Gのx座標が署名sと一致すればよい。
template <size_t N>
bool ecdsa_verify(const BigNum<N>& msg_hash, const ECDSASignature<N>& sig, const ECPoint<N>& public_key, const BigNum<N>& n, const BigNum<N>& a, const BigNum<N>& m, const ECPoint<N>& G) {
    // 0 < r < n, 0 < s < n のチェックが必要
    if (is_zero(sig.r) || is_zero(sig.s)) return false;

    // w = s^-1 % n
    BigNum<N> w = modinv(sig.s, n);

    // u1 = (msg_hash * s^-1) % n
    // u2 = (r * s^-1) % n
    BigNum<N> u1 = mod_mul(msg_hash, w, n);
    BigNum<N> u2 = mod_mul(sig.r, w, n);

    // X = u1*G + u2*Q
    ECPoint<N> pt1 = ec_mul(u1, G, a, m);
    ECPoint<N> pt2 = ec_mul(u2, public_key, a, m);
    ECPoint<N> X = ec_add(pt1, pt2, a, m);

    if (X.is_infinity){
        return false;
    }

    // Xのx座標 mod n をvとして、v = rなら検証成功
    BigNum<N> v = mod_mul(X.x, BigNum<N>{{1}}, n);
    return (compare<N>(v.digits.data(), sig.r.digits.data()) == 0);
}


int main() {
    constexpr size_t N = 8;
    BigNum<N> a, b, p, k, g_x, g_y;
    ECPoint<N> G = {g_x, g_y, false};

    std::cout << "\n--- ECDSA署名生成 ---" << std::endl;
    auto ecdsa_sign_start = std::chrono::high_resolution_clock::now();

    //P-256(secp256r1)の各種パラメータセット
    // p
    assign_from_bitstring(p, htob("FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF"));
    // a = -3
    assign_from_bitstring(a, htob("FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFC"));
    // Gx,Gy
    assign_from_bitstring(g_x, htob("6B17D1F2E12C4247F8BCE6E563A440F277037D812DEB33A0F4A13945D898C296"));
    assign_from_bitstring(g_y, htob("4FE342E2FE1A7F9B8EE7EB4A7C0F9E162BCE33576B315ECECBB6406837BF51F5"));
    G = {g_x, g_y, false};

    BigNum<N> n, d, msg, priv_key;
    // n
    assign_from_bitstring(n, htob("FFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551"));

    // 1. 秘密鍵 d と公開鍵 Q = d*G の作成
    assign_from_bitstring(priv_key, htob("DEBDB65664565454564564564564564564564564564564564564564564564564")); 
    ECPoint<N> Q = ec_mul(priv_key, G, a, p);

    // 2. メッセージハッシュ (適当な 256bit 値)
    assign_from_bitstring(msg, htob("4b8118f58b5906214253966e010e14bc0942e5352e85a53826019a16f6b5b5c9"));

    // 3. 署名
    ECDSASignature<N> sig = ecdsa_sign(msg, priv_key, n, a, p, G);
    std::cout << "k: " << to_hex(k) << std::endl;
    std::cout << "Signature r: " << to_hex(sig.r) << std::endl;
    std::cout << "Signature s: " << to_hex(sig.s) << std::endl;

    
    auto ecdsa_sign_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> ecdsa_sign_time = ecdsa_sign_end - ecdsa_sign_start;
    std::cout << "ECDSA sign time: " << ecdsa_sign_time.count() << " sec\n";

    // 4. 検証
    auto ecdsa_verify_start = std::chrono::high_resolution_clock::now();
    std::cout << "ECDSA署名検証" << std::endl;
    bool is_valid = ecdsa_verify(msg, sig, Q, n, a, p, G);
    std::cout << "Verify: " << (is_valid ? "署名一致" : "署名不一致") << std::endl;

    
    auto ecdsa_verify_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> ecdsa_verify_time = ecdsa_verify_end - ecdsa_verify_start;
    std::cout << "ECDSA verify time: " << ecdsa_verify_time.count() << " sec\n";
    return 0;
}