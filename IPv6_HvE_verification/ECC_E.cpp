#include "IPv6_HvE_verification.h"

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <memory> // 确保 unique_ptr 可用

// OpenSSL 头文件
#include <openssl/ec.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/err.h>
#include <openssl/rand.h> // **FIXED: 添加 RAND_bytes 所需的头文件**

// 使用标准命名空间，简化代码书写
using namespace std;

// -----------------------------------------------------------------
// 1. 错误处理和帮助函数
// -----------------------------------------------------------------

/**
 * @brief 检查 OpenSSL 错误并抛出异常。
 */
static void handle_openssl_error(const string& msg) {
    char err_buf[128];
    unsigned long err = ERR_get_error();
    ERR_error_string_n(err, err_buf, sizeof(err_buf));
    throw runtime_error(msg + ": " + string(err_buf));
}

/**
 * @brief Base64 编码 (使用 OpenSSL BIO)
 * @param data 要编码的字节数据
 * @return Base64 编码后的字符串
 */
string base64_encode(const vector<uint8_t>& data) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bio = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bio);

    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); // 不添加换行

    if (BIO_write(b64, data.data(), data.size()) <= 0) {
        BIO_free_all(b64);
        throw runtime_error("Base64 encode failed.");
    }
    BIO_flush(b64);

    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(b64, &bufferPtr);
    string encoded_string(bufferPtr->data, bufferPtr->length);

    BIO_free_all(b64);
    return encoded_string;
}

/**
 * @brief Base64 解码 (使用 OpenSSL BIO)
 * @param base64_str 要解码的 Base64 字符串
 * @return 解码后的字节数据
 */
vector<uint8_t> base64_decode(const string& base64_str) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bio = BIO_new_mem_buf(base64_str.data(), base64_str.length());
    b64 = BIO_push(b64, bio);

    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    // 预估最大解码尺寸
    vector<uint8_t> decoded_data(base64_str.length() * 3 / 4 + 1);
    int len = BIO_read(b64, decoded_data.data(), decoded_data.size());

    if (len <= 0) {
        BIO_free_all(b64);
        throw runtime_error("Base64 decode failed or resulted in empty data.");
    }

    decoded_data.resize(len);
    BIO_free_all(b64);
    return decoded_data;
}

// -----------------------------------------------------------------
// 2. ECC ECIES 核心实现类
// -----------------------------------------------------------------

/**
 * @brief ECIES 加解密管理类，使用 OpenSSL EVP_PKEY 封装密钥。
 */
class ECIESManager {
public:
    // 使用 std::unique_ptr 管理 OpenSSL 资源
    using UniquePKey = unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;

    ECIESManager() = default;

    /**
     * @brief 生成 ECC 密钥对。
     * @param curve_nid 曲线 NID，例如 NID_X9_62_prime256v1 (secp256r1)。
     * @return 包含公钥和私钥的 UniquePKey 对象。
     */
    UniquePKey generate_key_pair(int curve_nid = NID_X9_62_prime256v1) {
        // UniquePKey pkey(EVP_PKEY_new(), EVP_PKEY_free); // 移除这行，将在 keygen 成功后创建

        EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
        if (!pctx) handle_openssl_error("EVP_PKEY_CTX_new_id failed");

        // ... (参数生成部分与原始代码相同)

        EVP_PKEY* params = nullptr;
        if (EVP_PKEY_paramgen(pctx, &params) <= 0) handle_openssl_error("paramgen failed");

        EVP_PKEY_CTX_free(pctx); // 释放参数生成上下文

        pctx = EVP_PKEY_CTX_new(params, nullptr);
        EVP_PKEY_free(params);
        if (!pctx) handle_openssl_error("EVP_PKEY_CTX_new failed");

        if (EVP_PKEY_keygen_init(pctx) <= 0) handle_openssl_error("keygen_init failed");

        // **修正 E0158:**
        // 使用一个原始指针变量来接收新生成的密钥，它是 EVP_PKEY_keygen 期望的 EVP_PKEY** 的左值。
        EVP_PKEY* raw_pkey = nullptr;
        if (EVP_PKEY_keygen(pctx, &raw_pkey) <= 0) handle_openssl_error("keygen failed");

        EVP_PKEY_CTX_free(pctx);

        // 将所有权转移给 UniquePKey 并返回
        return UniquePKey(raw_pkey, EVP_PKEY_free);
    }

    /**
     * @brief 使用公钥加密数据 (ECIES)。
     * 密文结构: [临时公钥 (Q_ephem) | IV/Nonce | 密文 | 认证标签 (TAG)]
     * @param receiver_pubkey 接收方的公钥。
     * @param plaintext 要加密的原始数据。
     * @return Base64 编码的密文文本。
     */
    string encrypt(const UniquePKey& receiver_pubkey, const string& plaintext) {
        // 1. 生成临时 (Ephemeral) 密钥对 Q_ephem, d_ephem
        UniquePKey ephemeral_key = generate_key_pair();

        // 2. ECDH 密钥交换，生成共享秘密 Z = d_ephem * Q_receiver
        size_t secret_len = EVP_PKEY_size(receiver_pubkey.get());
        vector<uint8_t> shared_secret(secret_len);

        EVP_PKEY_CTX* dctx = EVP_PKEY_CTX_new(ephemeral_key.get(), nullptr);
        if (EVP_PKEY_derive_init(dctx) <= 0) handle_openssl_error("ECDH init failed");
        if (EVP_PKEY_derive_set_peer(dctx, receiver_pubkey.get()) <= 0) handle_openssl_error("ECDH set peer failed");

        // 确保 &secret_len 被正确传入
        if (EVP_PKEY_derive(dctx, shared_secret.data(), &secret_len) <= 0) handle_openssl_error("ECDH derive failed"); // 行 163 附近
        EVP_PKEY_CTX_free(dctx);

        // 3. KDF (Key Derivation Function): 从 Z 派生 AES 密钥 (K_E) 和 HMAC 密钥 (K_M)
        const size_t KEY_SIZE = 32; // 使用 256 位 AES-GCM
        const size_t IV_SIZE = 12;  // GCM 标准 Nonce 长度
        vector<uint8_t> derived_key(KEY_SIZE);

        EVP_PKEY_CTX* kdf_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);
        if (EVP_PKEY_derive_init(kdf_ctx) <= 0) handle_openssl_error("KDF init failed");
        if (EVP_PKEY_CTX_set_hkdf_md(kdf_ctx, EVP_sha256()) <= 0) handle_openssl_error("KDF set MD failed");
        if (EVP_PKEY_CTX_set1_hkdf_salt(kdf_ctx, nullptr, 0) <= 0) handle_openssl_error("KDF set salt failed");
        if (EVP_PKEY_CTX_set1_hkdf_key(kdf_ctx, shared_secret.data(), secret_len) <= 0) handle_openssl_error("KDF set key failed");
        if (EVP_PKEY_derive(dctx, shared_secret.data(), &secret_len) <= 0) handle_openssl_error("ECDH derive failed");
        EVP_PKEY_CTX_free(kdf_ctx);

        // 4. AES-256-GCM 加密
        vector<uint8_t> iv(IV_SIZE);
        if (RAND_bytes(iv.data(), IV_SIZE) <= 0) handle_openssl_error("RAND_bytes failed"); // 行 168 附近: 修复 E0020

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) <= 0) handle_openssl_error("EVP_EncryptInit failed");
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_SIZE, nullptr) <= 0) handle_openssl_error("EVP_CTRL_GCM_SET_IVLEN failed");
        if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, derived_key.data(), iv.data()) <= 0) handle_openssl_error("EVP_EncryptInit key/IV failed");

        int len;
        vector<uint8_t> ciphertext(plaintext.length() + EVP_MAX_IV_LENGTH); // 预留填充空间

        if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, reinterpret_cast<const uint8_t*>(plaintext.data()), plaintext.length()) <= 0) handle_openssl_error("EVP_EncryptUpdate failed");
        int ciphertext_len = len;

        if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) <= 0) handle_openssl_error("EVP_EncryptFinal failed");
        ciphertext_len += len;
        ciphertext.resize(ciphertext_len);

        vector<uint8_t> tag(16); // GCM 标签通常为 16 字节
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag.data()) <= 0) handle_openssl_error("EVP_CTRL_GCM_GET_TAG failed");
        EVP_CIPHER_CTX_free(ctx);

        // 5. 序列化临时公钥 Q_ephem
        int pub_key_len = i2d_PUBKEY(ephemeral_key.get(), nullptr);
        vector<uint8_t> ephemeral_pubkey_bytes(pub_key_len);
        uint8_t* p = ephemeral_pubkey_bytes.data();
        i2d_PUBKEY(ephemeral_key.get(), &p);

        // 6. 组合最终密文结构：[临时公钥 | IV | 密文 | Tag]
        vector<uint8_t> final_ciphertext;
        final_ciphertext.insert(final_ciphertext.end(), ephemeral_pubkey_bytes.begin(), ephemeral_pubkey_bytes.end());
        final_ciphertext.insert(final_ciphertext.end(), iv.begin(), iv.end());
        final_ciphertext.insert(final_ciphertext.end(), ciphertext.begin(), ciphertext.end());
        final_ciphertext.insert(final_ciphertext.end(), tag.begin(), tag.end());

        return base64_encode(final_ciphertext);
    }

    /**
     * @brief 使用私钥解密密文 (ECIES)。
     * @param receiver_privkey 接收方的私钥。
     * @param base64_ciphertext Base64 编码的密文文本。
     * @return 解密后的原始文本数据。
     */
    string decrypt(const UniquePKey& receiver_privkey, const string& base64_ciphertext) {
        // 1. Base64 解码
        vector<uint8_t> raw_ciphertext = base64_decode(base64_ciphertext);

        // 2. 解析密文：[临时公钥 | IV | 密文 | Tag]
        const size_t IV_SIZE = 12;
        const size_t TAG_SIZE = 16;

        const uint8_t* p = raw_ciphertext.data();
        UniquePKey ephemeral_pubkey(d2i_PUBKEY(nullptr, &p, raw_ciphertext.size()), EVP_PKEY_free);
        if (!ephemeral_pubkey) handle_openssl_error("d2i_PUBKEY failed (malformed public key in ciphertext)");

        size_t pubkey_len = p - raw_ciphertext.data(); // 临时公钥长度

        if (raw_ciphertext.size() < pubkey_len + IV_SIZE + TAG_SIZE) {
            throw runtime_error("Ciphertext too short or format incorrect.");
        }

        vector<uint8_t> iv(raw_ciphertext.begin() + pubkey_len, raw_ciphertext.begin() + pubkey_len + IV_SIZE);

        size_t cipher_start = pubkey_len + IV_SIZE;
        size_t cipher_end = raw_ciphertext.size() - TAG_SIZE;
        vector<uint8_t> ciphertext(raw_ciphertext.begin() + cipher_start, raw_ciphertext.begin() + cipher_end);

        vector<uint8_t> tag(raw_ciphertext.begin() + cipher_end, raw_ciphertext.end());

        // 3. ECDH 密钥交换，生成共享秘密 Z = d_receiver * Q_ephem
        size_t secret_len = EVP_PKEY_size(receiver_privkey.get());
        vector<uint8_t> shared_secret(secret_len);

        EVP_PKEY_CTX* dctx = EVP_PKEY_CTX_new(receiver_privkey.get(), nullptr);
        if (EVP_PKEY_derive_init(dctx) <= 0) handle_openssl_error("ECDH init failed");
        if (EVP_PKEY_derive_set_peer(dctx, ephemeral_pubkey.get()) <= 0) handle_openssl_error("ECDH set peer failed");

        // 确保 &secret_len 被正确传入
        if (EVP_PKEY_derive(dctx, shared_secret.data(), &secret_len) <= 0) handle_openssl_error("ECDH derive failed"); // 行 256 附近
        EVP_PKEY_CTX_free(dctx);

        // 4. KDF (Key Derivation Function): 从 Z 派生 AES 密钥 (K_E)
        const size_t KEY_SIZE = 32;
        vector<uint8_t> derived_key(KEY_SIZE);

        EVP_PKEY_CTX* kdf_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);
        if (EVP_PKEY_derive_init(kdf_ctx) <= 0) handle_openssl_error("KDF init failed");
        if (EVP_PKEY_CTX_set_hkdf_md(kdf_ctx, EVP_sha256()) <= 0) handle_openssl_error("KDF set MD failed");
        if (EVP_PKEY_CTX_set1_hkdf_salt(kdf_ctx, nullptr, 0) <= 0) handle_openssl_error("KDF set salt failed");
        if (EVP_PKEY_CTX_set1_hkdf_key(kdf_ctx, shared_secret.data(), secret_len) <= 0) handle_openssl_error("KDF set key failed");
        if (EVP_PKEY_derive(dctx, shared_secret.data(), &secret_len) <= 0) handle_openssl_error("ECDH derive failed");
        EVP_PKEY_CTX_free(kdf_ctx);

        // 5. AES-256-GCM 解密并认证
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) <= 0) handle_openssl_error("EVP_DecryptInit failed");
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_SIZE, nullptr) <= 0) handle_openssl_error("EVP_CTRL_GCM_SET_IVLEN failed");
        if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, derived_key.data(), iv.data()) <= 0) handle_openssl_error("EVP_DecryptInit key/IV failed");

        int len;
        vector<uint8_t> plaintext(ciphertext.size()); // 预留最大空间

        if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), ciphertext.size()) <= 0) handle_openssl_error("EVP_DecryptUpdate failed");
        int plaintext_len = len;

        // 设置 GCM Tag (认证)
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag.data()) <= 0) handle_openssl_error("EVP_CTRL_GCM_SET_TAG failed");

        // 最终解密 (如果认证失败，此函数返回 0)
        if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) <= 0) {
            EVP_CIPHER_CTX_free(ctx);
            throw runtime_error("Decryption failed: Authentication tag check failed (MAC check).");
        }
        plaintext_len += len;
        plaintext.resize(plaintext_len);

        EVP_CIPHER_CTX_free(ctx);

        // 6. 字节块转文本数据并返回
        return string(reinterpret_cast<const char*>(plaintext.data()), plaintext.size());
    }
};

// -----------------------------------------------------------------
// 3. 演示函数
// -----------------------------------------------------------------

//int main() {
//    // OpenSSL 初始化
//    ERR_load_crypto_strings();
//    OpenSSL_add_all_algorithms();
//
//    ECIESManager manager;
//    ECIESManager::UniquePKey receiver_privkey;
//
//    try {
//        // 1. 密钥生成 (接收方)
//        receiver_privkey = manager.generate_key_pair();
//
//        // 检查：确保密钥类型正确
//        if (EVP_PKEY_base_id(receiver_privkey.get()) != EVP_PKEY_EC) {
//            throw runtime_error("Key type is not ECC.");
//        }
//
//        cout << "--- ECC ECIES (OpenSSL) 演示 ---" << endl;
//        cout << "接收方密钥对生成成功 (Curve: secp256r1)." << endl;
//
//        // 2. 定义明文
//        string original_message = "这是一条使用 OpenSSL ECC ECIES 加密的安全消息。Hello from C++!";
//        cout << "\n原始明文:\n" << original_message << endl;
//
//        // 3. 加密 (发送方使用接收方公钥)
//        cout << "\n--- 开始加密 ---" << endl;
//        // 注意：这里使用 receiver_privkey.get() 是为了在 ECDH 中访问公钥点。
//        string base64_ciphertext = manager.encrypt(receiver_privkey, original_message);
//        cout << "加密密文 (Base64 文本):\n" << base64_ciphertext.substr(0, 80) << "..." << endl;
//
//        // 4. 解密 (接收方使用自己的私钥)
//        cout << "\n--- 开始解密 ---" << endl;
//        string recovered_message = manager.decrypt(receiver_privkey, base64_ciphertext);
//        cout << "解密明文:\n" << recovered_message << endl;
//
//        // 5. 验证
//        if (original_message == recovered_message) {
//            cout << "\n✅ 验证成功: 原始明文与解密明文一致。" << endl;
//        }
//        else {
//            cout << "\n❌ 验证失败: 明文不一致！" << endl;
//        }
//
//    }
//    catch (const exception& e) {
//        cerr << "\n加密/解密过程中发生错误: " << e.what() << endl;
//        return 1;
//    }
//
//    // OpenSSL 清理
//    EVP_cleanup();
//    CRYPTO_cleanup_all_ex_data();
//    ERR_free_strings();
//
//    return 0;
//}