#ifndef RSA_OPS_HPP
#define RSA_OPS_HPP

#include "bignum.hpp"
#include "rsa_utils.hpp"

// 逆元 (ECDSAのものと共通だが、RSA用に再掲/統合)
template <size_t N>
BigNum<N> modinv(BigNum<N> a, const BigNum<N>& m) {
    BigNum<N> b = m;
    BigNum<N> u{}, v{};
    u.digits[0] = 1;

    while (!is_zero(b)) {
        BigNum<N> t{}, r{};
        divT<N, N>(t.digits.data(), r.digits.data(), a.digits.data(), b.digits.data());

        uint64_t mulbuf[2 * N] = {};
        mulT<N>(mulbuf, t.digits.data(), v.digits.data());

        BigNum<2 * N> qtmp{};
        BigNum<N> tv{};
        divT<2 * N, N>(qtmp.digits.data(), tv.digits.data(), mulbuf, m.digits.data());

        BigNum<N> unsigned_u;
        if (compare<N>(u.digits.data(), tv.digits.data()) < 0) {
            addT<N>(unsigned_u.digits.data(), u.digits.data(), m.digits.data());
        } else {
            unsigned_u = u;
        }
        subT<N>(unsigned_u.digits.data(), unsigned_u.digits.data(), tv.digits.data());

        a = b;
        b = r;
        u = v;
        v = unsigned_u;
    }
    BigNum<N> q_dummy, r_res;
    divT<N, N>(q_dummy.digits.data(), r_res.digits.data(), u.digits.data(), m.digits.data());
    return r_res;
}

// べき剰余 (バイナリ法)
template <size_t N>
BigNum<N> modexp(BigNum<N> a, BigNum<N> b, const BigNum<N>& m) {
    BigNum<N> result{};
    result.digits[0] = 1;
    while (!is_zero(b)) {
        if (is_odd(b)) {
            uint64_t widebuf[2 * N] = {};
            mulT<N>(widebuf, result.digits.data(), a.digits.data());
            BigNum<2 * N> qtmp{};
            BigNum<N> rtmp{};
            divT<2 * N, N>(qtmp.digits.data(), rtmp.digits.data(), widebuf, m.digits.data());
            result = rtmp;
        }
        {
            uint64_t widebuf[2 * N] = {};
            mulT<N>(widebuf, a.digits.data(), a.digits.data());
            BigNum<2 * N> qtmp{};
            BigNum<N> rtmp{};
            divT<2 * N, N>(qtmp.digits.data(), rtmp.digits.data(), widebuf, m.digits.data());
            a = rtmp;
        }
        shift_right_one_bit<N>(b);
    }
    return result;
}

// CRT(中国剰余定理)を用いた復号
template <size_t N>
BigNum<N> decrypt_crt(const BigNum<N>& c, const BigNum<N>& p, const BigNum<N>& q, 
                      const BigNum<N>& dp, const BigNum<N>& dq, const BigNum<N>& q1) {
    // mp = c^dp mod p
    // mq = c^dq mod q
    BigNum<N> mp = modexp(c, dp, p);
    BigNum<N> mq = modexp(c, dq, q);

    // h = (mp - mq) * qinv mod p
    BigNum<N> h_diff;
    if (compare<N>(mp.digits.data(), mq.digits.data()) < 0) {
        addT<N>(h_diff.digits.data(), mp.digits.data(), p.digits.data());
        subT<N>(h_diff.digits.data(), h_diff.digits.data(), mq.digits.data());
    } else {
        subT<N>(h_diff.digits.data(), mp.digits.data(), mq.digits.data());
    }

    uint64_t widebuf_h[N * 2] = {};
    mulT<N>(widebuf_h, h_diff.digits.data(), q1.digits.data());

    BigNum<N> h, q_dummy_h;
    divT<N * 2, N>(q_dummy_h.digits.data(), h.digits.data(), widebuf_h, p.digits.data());

    // m = mq + h * q
    uint64_t widebuf_final[N * 2] = {};
    mulT<N>(widebuf_final, h.digits.data(), q.digits.data());

    BigNum<N> final_m;
    std::memcpy(final_m.digits.data(), widebuf_final, sizeof(uint64_t) * N);
    addT<N>(final_m.digits.data(), final_m.digits.data(), mq.digits.data());

    return final_m;
}

#endif