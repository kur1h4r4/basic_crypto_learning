#include <iostream>
#include <chrono>
#include <string>
#include <cstring>

#include "bignum.hpp"
#include "rsa_utils.hpp"
#include "rsa_ops.hpp"

int main() {
    auto total_start = std::chrono::high_resolution_clock::now();
    
    constexpr size_t BASE_N = 64;
    BigNum<BASE_N> p, q, n, d, e, m, c, pminus, qminus, toshent, decrypt_m, one;
    std::string pbits, qbits, ebits, mbits;

    // 2048bit パラメータ
    pbits = htob("efebb1728416286c7f6106c64809881f5e099bf39a2b6e1ab265cb08756e8d4ea164e109638ff282f63f0d22289e2fb11aee07dcc241a4dba4e296b177ddabd9b53ab541decd495f5a10a2157880689139ce735affeaac076cf390d45a61cf72efc2a7bab9aabd8d40ac8f5020a4d5e7d5ea304e46cb68ba2ac3e6ae28033535");
    qbits = htob("d0d1e47cca884cb9aeb9718b7505d50fa5b2f6a6612a209af367f9dfcf13a0a8cbce19f6f96859992631a36ac88ea3b4828be2112413e78651b775570b6ade962a3ee7d1cb4d32bcd4b8ffdd98cd2fef6d3f24c770ac3642d9006c31e305fdffbde8e33033a5919f264c27fcb645430e130c833b1504593da8967a6bbdf09c3f");
    ebits = htob("10001");
    mbits = htob("ff");

    assign_from_bitstring(p, pbits);
    assign_from_bitstring(q, qbits);
    assign_from_bitstring(e, ebits);
    assign_from_bitstring(m, mbits);
    assign_from_bitstring(one, "1");

    // n = p * q
    uint64_t widebuf[BASE_N * 2] = {};
    mulT<BASE_N>(widebuf, p.digits.data(), q.digits.data());
    std::memcpy(n.digits.data(), widebuf, sizeof(uint64_t) * BASE_N);

    // φ = (p-1)*(q-1)
    subT<BASE_N>(pminus.digits.data(), p.digits.data(), one.digits.data());
    subT<BASE_N>(qminus.digits.data(), q.digits.data(), one.digits.data());
    std::memset(widebuf, 0, sizeof(widebuf));
    mulT<BASE_N>(widebuf, pminus.digits.data(), qminus.digits.data());
    std::memcpy(toshent.digits.data(), widebuf, sizeof(uint64_t) * BASE_N);

    // 暗号化
    c = modexp(m, e, n);
    std::cout << "c (hex) = " << to_hex(c) << std::endl;

    // 秘密鍵 D
    d = modinv(e, toshent);
    std::cout << "d (hex) = " << to_hex(d) << std::endl;

    // 平文 M の算出 (単純なべき剰余方式)
    decrypt_m = modexp(c, d, n);
    std::cout << "decrypt_m (hex) = " << to_hex(decrypt_m) << std::endl;

    auto total_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = total_end - total_start;
    std::cout << "Total time: " << diff.count() << " sec\n";

    return 0;
}