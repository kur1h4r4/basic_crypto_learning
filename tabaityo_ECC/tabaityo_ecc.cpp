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
template <size_t N>
struct ECPoint {
    BigNum<N> x, y;
    bool is_infinity = false; // 無限遠点フラグ
};

// 点加算 (P + Q)
template <size_t N>
ECPoint<N> ec_add(const ECPoint<N>& P, const ECPoint<N>& Q, const BigNum<N>& a, const BigNum<N>& p) {
    if (P.is_infinity) return Q;
    if (Q.is_infinity) return P;

    // x座標が同じ場合
    if (compare<N>(P.x.digits.data(), Q.x.digits.data()) == 0) {
        // yが互いに逆（P = -Q）なら無限遠点
        BigNum<N> neg_y = mod_sub(p, Q.y, p);
        if (compare<N>(P.y.digits.data(), neg_y.digits.data()) == 0 || is_zero(P.y)) {
            return {BigNum<N>(), BigNum<N>(), true};
        }
        // 点倍算 (P + P)
        // lambda = (3x^2 + a) / 2y
        BigNum<N> three{}, two{};
        three.digits[0] = 3; two.digits[0] = 2;

        BigNum<N> num = mod_add(mod_mul(mod_mul(P.x, P.x, p), three, p), a, p);
        BigNum<N> den = modinv(mod_mul(P.y, two, p), p);
        BigNum<N> lambda = mod_mul(num, den, p);

        BigNum<N> x3 = mod_sub(mod_sub(mod_mul(lambda, lambda, p), P.x, p), P.x, p);
        BigNum<N> y3 = mod_sub(mod_mul(lambda, mod_sub(P.x, x3, p), p), P.y, p);
        return {x3, y3, false};
    } else {
        // 通常の加算
        // lambda = (y2 - y1) / (x2 - x1)
        BigNum<N> num = mod_sub(Q.y, P.y, p);
        BigNum<N> den = modinv(mod_sub(Q.x, P.x, p), p);
        BigNum<N> lambda = mod_mul(num, den, p);

        BigNum<N> x3 = mod_sub(mod_sub(mod_mul(lambda, lambda, p), P.x, p), Q.x, p);
        BigNum<N> y3 = mod_sub(mod_mul(lambda, mod_sub(P.x, x3, p), p), P.y, p);
        return {x3, y3, false};
    }
}

// スカラー倍 (k * P) - バイナリ法（Double-and-Add）
template <size_t N>
ECPoint<N> ec_mul(BigNum<N> k, ECPoint<N> P, const BigNum<N>& a, const BigNum<N>& p) {
    ECPoint<N> result = {BigNum<N>(), BigNum<N>(), true}; // 無限遠点で初期化
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

// 署名生成
template <size_t N>
ECDSASignature<N> ecdsa_sign(const BigNum<N>& msg_hash, const BigNum<N>& private_key, 
                             const BigNum<N>& n, const BigNum<N>& a, const BigNum<N>& p, 
                             const ECPoint<N>& G) {
    // 実際にはセキュアな乱数生成器が必要ですが、ここではテスト用に固定値や簡易乱数で代用を検討してください
    // 本来はループ内で r != 0 かつ s != 0 になるまで k を選び直します
    
    // テスト用の一次的な k (本来は 0 < k < n)
    BigNum<N> k;
    k.digits[0] = 0x12345678; // 簡易的な例

    // 1. R = k * G
    ECPoint<N> R = ec_mul(k, G, a, p);
    
    // 2. r = R.x % n
    BigNum<N> r = mod_mul(R.x, BigNum<N>{{1}}, n); // mod_mulを使って n で割った余りを得る
    if (is_zero(r)) return ecdsa_sign(msg_hash, private_key, n, a, p, G); // 再試行

    // 3. s = k^-1 * (msg_hash + r * private_key) % n
    BigNum<N> k_inv = modinv(k, n);
    BigNum<N> r_d = mod_mul(r, private_key, n);
    BigNum<N> e_rd = mod_add(msg_hash, r_d, n);
    BigNum<N> s = mod_mul(k_inv, e_rd, n);

    if (is_zero(s)) return ecdsa_sign(msg_hash, private_key, n, a, p, G); // 再試行

    return {r, s};
}

// 署名検証
template <size_t N>
bool ecdsa_verify(const BigNum<N>& msg_hash, const ECDSASignature<N>& sig, 
                  const ECPoint<N>& public_key, const BigNum<N>& n, 
                  const BigNum<N>& a, const BigNum<N>& p, const ECPoint<N>& G) {
    // 0 < r < n, 0 < s < n のチェックが必要
    if (is_zero(sig.r) || is_zero(sig.s)) return false;

    // 1. w = s^-1 % n
    BigNum<N> w = modinv(sig.s, n);

    // 2. u1 = (msg_hash * w) % n, u2 = (r * w) % n
    BigNum<N> u1 = mod_mul(msg_hash, w, n);
    BigNum<N> u2 = mod_mul(sig.r, w, n);

    // 3. X = u1*G + u2*Q
    ECPoint<N> pt1 = ec_mul(u1, G, a, p);
    ECPoint<N> pt2 = ec_mul(u2, public_key, a, p);
    ECPoint<N> X = ec_add(pt1, pt2, a, p);

    if (X.is_infinity) return false;

    // 4. v = X.x % n. v == r なら正当
    BigNum<N> v = mod_mul(X.x, BigNum<N>{{1}}, n);
    return (compare<N>(v.digits.data(), sig.r.digits.data()) == 0);
}


// 高速化せず愚直にECDSA署名を実装すると通常の100倍くらい時間がかかる。
int main() {
    constexpr size_t N = 64;
    BigNum<N> a, b, p, k, g_x, g_y;
    ECPoint<N> G = {g_x, g_y, false};

    std::cout << "\n--- ECDSA署名生成 ---" << std::endl;
    auto ecdsa_sign_start = std::chrono::high_resolution_clock::now();

    // p = FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE FFFFFC2F
    assign_from_bitstring(p, htob("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F"));
    assign_from_bitstring(a, htob("00")); // a=0
    // G = (79BE667E F9DCBBAC 55A06295 CE870B07 029BFCDB 2DCE28D9 59F2815B 16F81798, ...)
    assign_from_bitstring(g_x, htob("79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798"));
    assign_from_bitstring(g_y, htob("483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8"));
    G = {g_x, g_y, false};

    BigNum<N> n, d, msg, priv_key;
    // secp256k1 の n (オーダー)
    assign_from_bitstring(n, htob("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141"));

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
    std::cout << "Verify: " << (is_valid ? "SUCCESS" : "FAILED") << std::endl;

    
    auto ecdsa_verify_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> ecdsa_verify_time = ecdsa_verify_end - ecdsa_verify_start;
    std::cout << "ECDSA verify time: " << ecdsa_verify_time.count() << " sec\n";
    return 0;
}