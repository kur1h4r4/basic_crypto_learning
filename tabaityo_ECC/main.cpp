#include <random>
#include <iostream>
#include <cstring>
#include <chrono>
#include <vector>
#include <array>
#include <string>
#include <cstdint>

// ヘッダーのインポート
#include "bignum.hpp"
#include "rsa_utils.hpp"
#include "ecdsa_ops.hpp"

int main() {
    constexpr size_t N = 8;
    BigNum<N> a, b, p, k, g_x, g_y;
    ECPoint<N> G = {g_x, g_y, false};

    auto ecdsa_sign_start = std::chrono::high_resolution_clock::now();

    // P-256(secp256r1)の各種パラメータセット
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

    // 秘密鍵 d と公開鍵 Q = d*G
    assign_from_bitstring(priv_key, htob("DEBDB65664565454564564564564564564564564564564564564564564564564")); 
    ECPoint<N> Q = ec_mul(priv_key, G, a, p);

    // 平文
    assign_from_bitstring(msg, htob("2121"));

    // 署名
    std::cout << "ECDSA署名生成" << std::endl;
    ECDSASignature<N> sig = ecdsa_sign(msg, priv_key, n, a, p, G);
    std::cout << "Signature r: " << to_hex(sig.r) << std::endl;
    std::cout << "Signature s: " << to_hex(sig.s) << std::endl;    
    auto ecdsa_sign_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> ecdsa_sign_time = ecdsa_sign_end - ecdsa_sign_start;
    std::cout << "ECDSA署名生成時間: " << ecdsa_sign_time.count() << " sec\n\n";

    // 署名検証
    auto ecdsa_verify_start = std::chrono::high_resolution_clock::now();
    std::cout << "ECDSA署名検証" << std::endl;
    bool is_valid = ecdsa_verify(msg, sig, Q, n, a, p, G);
    std::cout << "Verify: " << (is_valid ? "署名一致" : "署名不一致") << std::endl;
    auto ecdsa_verify_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> ecdsa_verify_time = ecdsa_verify_end - ecdsa_verify_start;
    std::cout << "ECDSA署名検証時間: " << ecdsa_verify_time.count() << " sec\n";
    return 0;
}