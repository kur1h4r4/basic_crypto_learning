#ifndef ECDSA_OPS_HPP
#define ECDSA_OPS_HPP

#include "bignum.hpp"
#include "rsa_utils.hpp"
#include <random>

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

    // 0 < k < nとしたい。
    // よって、nが何ワード（uint64_t配列いくつ分）あるか計算する
    // bit_length / 64 を切り上げれば、有効な配列サイズがわかる
    int n_bits = bit_length_uint64(n.digits.data(), N);
    size_t active_words = (n_bits + 63) / 64; 

    BigNum<N> k;
    while (true) {
        k.digits.fill(0);

        // nのワード数分だけランダムに埋める
        for (size_t i = 0; i < active_words; ++i) {
            k.digits[i] = dist(gen);
        }

        // 0 < k かチェック
        bool all_zero = true;
        for (size_t i = 0; i < active_words; ++i) {
            if (k.digits[i] != 0) {
                all_zero = false;
            }
        }

        if (all_zero) {
            continue;
        }

        // k < n のチェック
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
    if (is_zero(sig.r) || is_zero(sig.s)){
        return false;
    }

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

#endif