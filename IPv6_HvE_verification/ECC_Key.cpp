#include "IPv6_HvE_verification.h"

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <memory>
#include <fstream> // 添加文件流头文件
#include <openssl/ec.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/pem.h>

// 使用标准命名空间
using namespace std;

// -----------------------------------------------------------------
// 1. 别名和错误处理
// -----------------------------------------------------------------

// 定义 unique_ptr 别名，用于自动管理 OpenSSL 资源
// UniquePKey 负责管理 EVP_PKEY*，并在析构时调用 EVP_PKEY_free 释放资源。
using UniquePKey = unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;

/**
 * @brief 检查 OpenSSL 错误并抛出异常。
 */
static void handle_openssl_error(const string& msg) {
    char err_buf[128];
    unsigned long err = ERR_get_error();
    ERR_error_string_n(err, err_buf, sizeof(err_buf));
    throw runtime_error(msg + ": " + string(err_buf));
}

// -----------------------------------------------------------------
// 2. 密钥生成器类
// -----------------------------------------------------------------

class ECKeyGenerator {
public:
    /**
     * @brief 构造函数，初始化 OpenSSL 环境。
     */
    ECKeyGenerator() {
        // 确保 OpenSSL 库初始化 (尽管现代版本通常自动完成，保留以防万一)
        // ERR_load_crypto_strings(); 
        // OpenSSL_add_all_algorithms();
    }

    /**
     * @brief 生成 ECC 密钥对。
     * 默认使用 NIST P-256 (secp256r1) 曲线。
     * @param curve_nid 曲线的 NID (Numerical ID)。
     * @return 包含公钥和私钥的 UniquePKey 智能指针。
     */
    UniquePKey generate_key_pair(int curve_nid = NID_X9_62_prime256v1) {
        // 1. 创建参数生成上下文 (PKEY_CTX)
        EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
        if (!pctx) handle_openssl_error("EVP_PKEY_CTX_new_id failed");

        // 2. 初始化参数生成过程
        if (EVP_PKEY_paramgen_init(pctx) <= 0) {
            EVP_PKEY_CTX_free(pctx);
            handle_openssl_error("paramgen_init failed");
        }

        // 3. 设置 ECC 曲线
        if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, curve_nid) <= 0) {
            EVP_PKEY_CTX_free(pctx);
            handle_openssl_error("set_ec_paramgen_curve_nid failed");
        }

        // 4. 生成参数
        EVP_PKEY* params = nullptr;
        if (EVP_PKEY_paramgen(pctx, &params) <= 0) {
            EVP_PKEY_CTX_free(pctx);
            handle_openssl_error("paramgen failed");
        }

        EVP_PKEY_CTX_free(pctx); // 释放参数生成上下文

        // 5. 创建密钥生成上下文 (基于已生成的参数)
        pctx = EVP_PKEY_CTX_new(params, nullptr);
        EVP_PKEY_free(params); // 参数已复制到 pctx，可以释放
        if (!pctx) handle_openssl_error("EVP_PKEY_CTX_new failed (for keygen)");

        // 6. 初始化密钥生成
        if (EVP_PKEY_keygen_init(pctx) <= 0) {
            EVP_PKEY_CTX_free(pctx);
            handle_openssl_error("keygen_init failed");
        }

        // 7. 生成密钥对
        EVP_PKEY* raw_pkey = nullptr;
        // EVP_PKEY_keygen 的第二个参数需要 EVP_PKEY**，用于写入新生成的密钥地址
        if (EVP_PKEY_keygen(pctx, &raw_pkey) <= 0) {
            EVP_PKEY_CTX_free(pctx);
            handle_openssl_error("EVP_PKEY_keygen failed");
        }

        EVP_PKEY_CTX_free(pctx);

        // 8. 将所有权转移给 UniquePKey 并返回
        return UniquePKey(raw_pkey, EVP_PKEY_free);
    }

    /**
     * @brief 将 EVP_PKEY 导出为 PEM 格式的字符串。
     * @param pkey 要导出的密钥。
     * @param is_private 如果为 true，导出私钥 (PEM_write_bio_PrivateKey)；否则导出公钥 (PEM_write_bio_PUBKEY)。
     * @return PEM 格式的字符串。
     */
    string export_to_pem(const UniquePKey& pkey, bool is_private) {
        if (!pkey) return "";

        // 创建 BIO 对象
        unique_ptr<BIO, decltype(&BIO_free_all)> bio(BIO_new(BIO_s_mem()), BIO_free_all);
        if (!bio) handle_openssl_error("BIO_new failed");

        int ret;
        if (is_private) {
            // 导出私钥 (包含公钥部分)
            ret = PEM_write_bio_PrivateKey(bio.get(), pkey.get(), NULL, NULL, 0, NULL, NULL);
        }
        else {
            // 导出公钥
            ret = PEM_write_bio_PUBKEY(bio.get(), pkey.get());
        }

        if (ret <= 0) handle_openssl_error("PEM_write_bio failed");

        // 从 BIO 中读取数据
        BUF_MEM* mem = nullptr;
        BIO_get_mem_ptr(bio.get(), &mem);
        if (mem && mem->data) {
            return string(mem->data, mem->length);
        }

        return "";
    }
};

/**
 * @brief 将字符串内容写入文件。
 * @param filename 要创建或覆盖的文件名。
 * @param content 要写入的内容。
 */
void write_to_file(const string& filename, const string& content) {
    ofstream ofs(filename);
    if (!ofs.is_open()) {
        throw runtime_error("无法打开文件进行写入: " + filename);
    }
    ofs << content;
    ofs.close();
}


// -----------------------------------------------------------------
// 3. 演示函数
// -----------------------------------------------------------------
/*
int main() {
    try {
        cout << "--- ECC 密钥对生成器演示 (基于 OpenSSL) ---" << endl;
        ECKeyGenerator generator;

        // 1. 生成密钥对 (默认为 secp256r1)
        UniquePKey key_pair = generator.generate_key_pair();
        if (!key_pair) {
            throw runtime_error("密钥对生成失败。");
        }

        cout << "\n✅ 密钥对生成成功 (曲线: secp256r1)" << endl;

        // 2. 导出私钥 (PEM 格式)
        string private_pem = generator.export_to_pem(key_pair, true);

        // 3. 导出公钥 (PEM 格式)
        string public_pem = generator.export_to_pem(key_pair, false);

        // 4. 将密钥写入文件
        const string private_file = "private_key.pem";
        const string public_file = "public_key.pem";

        write_to_file(private_file, private_pem);
        write_to_file(public_file, public_pem);

        cout << "\n--- 密钥文件已保存 ---" << endl;
        cout << "私钥已保存到: " << private_file << endl;
        cout << "公钥已保存到: " << public_file << endl;

        // 5. 仍将内容打印到控制台以供预览
        cout << "\n--- 生成的私钥 (预览) ---" << endl;
        cout << private_pem.substr(0, 100) << "..." << endl;
        cout << "\n--- 生成的公钥 (预览) ---" << endl;
        cout << public_pem.substr(0, 100) << "..." << endl;


    }
    catch (const exception& e) {
        cerr << "\n❌ 发生错误: " << e.what() << endl;
        return 1;
    }

    return 0;
}
*/