#include <random>
#include <iostream>
#include <cstring>
#include <random>
#include <chrono>
#include <openssl/bn.h>
#include <openssl/rsa.h>

int main(){   
    auto total_start = std::chrono::high_resolution_clock::now();

    const char* p_hex = "efebb1728416286c7f6106c64809881f5e099bf39a2b6e1ab265cb08756e8d4ea164e109638ff282f63f0d22289e2fb11aee07dcc241a4dba4e296b177ddabd9b53ab541decd495f5a10a2157880689139ce735affeaac076cf390d45a61cf72efc2a7bab9aabd8d40ac8f5020a4d5e7d5ea304e46cb68ba2ac3e6ae28033535";
    const char* q_hex = "d0d1e47cca884cb9aeb9718b7505d50fa5b2f6a6612a209af367f9dfcf13a0a8cbce19f6f96859992631a36ac88ea3b4828be2112413e78651b775570b6ade962a3ee7d1cb4d32bcd4b8ffdd98cd2fef6d3f24c770ac3642d9006c31e305fdffbde8e33033a5919f264c27fcb645430e130c833b1504593da8967a6bbdf09c3f";

    const char* e_hex = "10001";
    const char* m_hex = "ff";

    BIGNUM* p = BN_new();
    BIGNUM* q = BN_new();
    BIGNUM* e = BN_new();
    BIGNUM* m = BN_new();

    BN_hex2bn(&p, p_hex);
    BN_hex2bn(&q, q_hex);
    BN_hex2bn(&e, e_hex);
    BN_hex2bn(&m, m_hex);

    
    // p * q = n
    BIGNUM* n = BN_new();
    BN_CTX *rsa_ctx = BN_CTX_new();
    BN_mul(n, p, q, rsa_ctx);


    //rsa暗号化
    BIGNUM* c = BN_new();
    BN_mod_exp(c, m, e, n, rsa_ctx);

    // dはeの逆元、modとーしぇんと
    BIGNUM* one = BN_new();
    BN_set_word(one, 1);

    BIGNUM* pminus = BN_new();
    BIGNUM* qminus = BN_new();
    BIGNUM* toshent = BN_new();

    BN_sub(pminus, p, one);
    BN_sub(qminus, q, one);
    BN_mul(toshent, pminus, qminus, rsa_ctx);

    // d = e^{-1} mod φ(n)
    BIGNUM* d = BN_mod_inverse(NULL, e, toshent, rsa_ctx);

    //rsa復号
    BIGNUM* decrypted_m = BN_new();
    BN_mod_exp(decrypted_m, c, d, n, rsa_ctx);

    
    char* hex_n = BN_bn2hex(n);
    char* hex_c = BN_bn2hex(c);
    char* hex_m = BN_bn2hex(decrypted_m);

    std::cout << "Original m " << BN_bn2hex(m) << std::endl;
    std::cout << "Encrypted n " << hex_n << std::endl;
    std::cout << "Encrypted c " << hex_c << std::endl;
    std::cout << "Decrypted m " << hex_m << std::endl;
    std::cout.flush();

    auto total_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = total_end - total_start;
    std::cout << "Total time: " << diff.count() << " sec\n";

    OPENSSL_free(hex_n);
    OPENSSL_free(hex_c);
    OPENSSL_free(hex_m);

    // opensslの多倍長整数の後始末
    BN_free(p);
    BN_free(q);
    BN_free(d);
    BN_free(e);
    BN_free(m);
    BN_free(n);
    BN_free(c);
    BN_free(decrypted_m);
    BN_CTX_free(rsa_ctx);

    

    // mod_expなどを呼び出せば計算はできるが、高速化はすでにされているっぽい。
    // 参考：https//github.com/openssl/openssl/blob/master/crypto/bn/bn_mul.c

    
}