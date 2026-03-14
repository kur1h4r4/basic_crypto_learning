#include <iostream>
#include <chrono>
#include <string>
#include <cstring>

#include "bignum.hpp"
#include "rsa_utils.hpp"
#include "rsa_ops.hpp"

int main() {
    auto total_start = std::chrono::high_resolution_clock::now();
    
    // 2048bit RSAの場合、BASE_N = 2048/64 = 32。
    // mulTで2倍必要なので 64 を確保。
    constexpr size_t BASE_N = 64;
    BigNum<BASE_N> p, q, n, d, e, m, c, pminus, qminus, toshent, one;
    BigNum<BASE_N> dp, dq, q1;
    std::string pbits, qbits, ebits, mbits;

    // 2048bit パラメータ (ご提示の値をそのまま使用)
    pbits = htob("efebb1728416286c7f6106c64809881f5e099bf39a2b6e1ab265cb08756e8d4ea164e109638ff282f63f0d22289e2fb11aee07dcc241a4dba4e296b177ddabd9b53ab541decd495f5a10a2157880689139ce735affeaac076cf390d45a61cf72efc2a7bab9aabd8d40ac8f5020a4d5e7d5ea304e46cb68ba2ac3e6ae28033535");
    qbits = htob("d0d1e47cca884cb9aeb9718b7505d50fa5b2f6a6612a209af367f9dfcf13a0a8cbce19f6f96859992631a36ac88ea3b4828be2112413e78651b775570b6ade962a3ee7d1cb4d32bcd4b8ffdd98cd2fef6d3f24c770ac3642d9006c31e305fdffbde8e33033a5919f264c27fcb645430e130c833b1504593da8967a6bbdf09c3f");
    ebits = htob("10001");
    mbits = htob("ff");

    assign_from_bitstring(p, pbits);
    assign_from_bitstring(q, qbits);
    assign_from_bitstring(e, ebits);
    assign_from_bitstring(m, mbits);
    assign_from_bitstring(one, "1");

    std::cout << "p (hex) = " << to_hex(p) << std::endl;
    std::cout << "q (hex) = " << to_hex(q) << std::endl;
    std::cout << "e (hex) = " << to_hex(e) << std::endl;

    // n = p * q
    uint64_t widebuf[BASE_N * 2] = {};
    mulT<BASE_N>(widebuf, p.digits.data(), q.digits.data());
    std::memcpy(n.digits.data(), widebuf, sizeof(uint64_t) * BASE_N);
    std::cout << "n (hex) = " << to_hex(n) << std::endl;
    
    // φ = (p-1)*(q-1)
    subT<BASE_N>(pminus.digits.data(), p.digits.data(), one.digits.data());
    subT<BASE_N>(qminus.digits.data(), q.digits.data(), one.digits.data());
    std::memset(widebuf, 0, sizeof(widebuf));
    mulT<BASE_N>(widebuf, pminus.digits.data(), qminus.digits.data());
    std::memcpy(toshent.digits.data(), widebuf, sizeof(uint64_t) * BASE_N);

    // 暗号化: C = M^E mod N
    c = modexp(m, e, n);
    std::cout << "c (hex) = " << to_hex(c) << std::endl;

    // 秘密鍵: D = E^-1 mod φ
    d = modinv(e, toshent);
    std::cout << "d (hex) = " << to_hex(d) << std::endl;

    // CRT用パラメータ計算
    BigNum<BASE_N> q_dummy;
    divT<BASE_N, BASE_N>(q_dummy.digits.data(), dp.digits.data(), d.digits.data(), pminus.digits.data());
    divT<BASE_N, BASE_N>(q_dummy.digits.data(), dq.digits.data(), d.digits.data(), qminus.digits.data());
    q1 = modinv(q, p);

    // CRTによる復号
    BigNum<BASE_N> final_m = decrypt_crt(c, p, q, dp, dq, q1);
    std::cout << "CRT decrypt_m (hex) = " << to_hex(final_m) << std::endl;

    auto total_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = total_end - total_start;
    std::cout << "Total time: " << diff.count() << " sec\n";

    return 0;
}