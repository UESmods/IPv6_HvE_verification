#include "IPv6_HvE_verification.h"

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <memory>
#include <iomanip>

// OpenSSL 头文件
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>

// 使用标准命名空间，简化代码书写
using namespace std;

const size_t AES_256_KEY_SIZE = 32;

/**
 * @brief 使用 OpenSSL 的 RAND_bytes() 生成一个密码学安全的 256 位 AES 密钥。
 *
 * @return vector<uint8_t> 包含 32 字节密钥的向量。
 */
vector<uint8_t> generate_aes_256_key_openssl() {
    vector<uint8_t> key(AES_256_KEY_SIZE);

    // RAND_bytes 返回 1 表示成功，<= 0 表示失败
    int result = RAND_bytes(key.data(), AES_256_KEY_SIZE);

    if (result != 1) {
        // 密钥生成失败
        unsigned long err = ERR_get_error();
        stringstream ss;
        ss << "OpenSSL RAND_bytes failed to generate key. Error code: " << err;

        // 获取更详细的错误信息
        char err_buf[256];
        ERR_error_string_n(err, err_buf, sizeof(err_buf));
        ss << " (" << err_buf << ")";

        throw runtime_error(ss.str());
    }

    return key;
}

//AES_256_iv生成函数
vector<uint8_t> generate_secure_iv(size_t size) {
    vector<uint8_t> iv(size);
    if (RAND_bytes(iv.data(), size) != 1) {
        throw runtime_error("无法生成安全的 IV：RAND_bytes 失败");
    }
    return iv;
}

// -----------------------------------------------------------------
// 1. 结构体定义 (与您要求保持一致)
// -----------------------------------------------------------------

// 启用 1 字节对齐，以确保字段之间没有编译器自动插入的填充
#pragma pack(push, 1)

/**
 * @brief HVE (Host-Verified Encryption) IPv6 扩展头结构体定义。
 * 总长度为 115 字节，通常在网络上传输时会填充到 120 字节。
 */
struct HVE_ExtensionHeader {
    uint8_t next_header;    // 下一头部 (1 字节)
    uint8_t eh_length;      // EH 长度 (1 字节)
    uint8_t r_row_index;    // R 行索引 (1 字节)

    // ECC密文+IV 64字节：这里为了简化GCM演示，我们将这64字节分配给 IV + 密文。
    // GCM IV/Nonce: 12 字节 (标准推荐)
    // 剩余空间给密文 (64 - 12 = 52 字节)
    static constexpr size_t GCM_IV_SIZE = 12;
    static constexpr size_t GCM_CIPHERTEXT_MAX_SIZE = 52;

    uint8_t iv[GCM_IV_SIZE]; // GCM IV/Nonce (12 字节)
    uint8_t ciphertext_body[GCM_CIPHERTEXT_MAX_SIZE]; // 密文主体 (52 字节)

    uint8_t gcm_mac_tag[16]; // GCM MAC标签 (16 字节)

    uint8_t padding_32_bytes[32]; // 自动对齐填充 (32 字节)

    static constexpr size_t TOTAL_SIZE =
        1 + 1 + 1 + GCM_IV_SIZE + GCM_CIPHERTEXT_MAX_SIZE + 16 + 32; // 115 字节

    // IPv6 EH 长度字段的值应为 (TOTAL_SIZE_ALIGNED / 8) - 1
    // (120 / 8) - 1 = 14
    static constexpr uint8_t EH_LEN_VALUE = 14;
};

// 恢复默认的字节对齐设置
#pragma pack(pop)

// -----------------------------------------------------------------
// 2. 错误处理和工具函数
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
 * @brief 将字节数据以十六进制格式打印到控制台。
 */
void print_hex(const string& label, const uint8_t* data, size_t len) {
    cout << label << " (" << len << " bytes): ";
    for (size_t i = 0; i < len; ++i) {
        cout << hex << setw(2) << setfill('0') << (int)data[i];
    }
    cout << dec << endl;
}

// -----------------------------------------------------------------
// 3. GCM 加密和MAC生成函数
// -----------------------------------------------------------------

/**
 * @brief 使用 AES-256 GCM 对数据进行加密，并生成 MAC 标签。
 * * @param plaintext 要加密的原始数据。
 * @param key 加密密钥 (32 字节)。
 * @param iv 初始化向量 (12 字节)。
 * @param aad 附加认证数据 (通常是IPv6基础头等不可变部分)。
 * @param ciphertext 密文输出缓冲区。
 * @param tag MAC 标签输出缓冲区 (16 字节)。
 * @return 密文的实际长度。
 */
int aes_256_gcm_encrypt(
    const vector<uint8_t>& plaintext,
    const uint8_t* key,
    const uint8_t* iv,
    const vector<uint8_t>& aad,
    vector<uint8_t>& ciphertext,
    uint8_t* tag)
{
    EVP_CIPHER_CTX* ctx = nullptr;
    int len = 0;
    int ciphertext_len = 0;

    // 创建和初始化上下文
    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) handle_openssl_error("EVP_CIPHER_CTX_new failed");

    // 初始化加密操作，使用 AES 256 GCM
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr))
        handle_openssl_error("EVP_EncryptInit_ex failed (init)");

    // 设置 IV/Nonce 长度 (GCM 模式下 IV/Nonce 长度推荐为 12 字节)
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, HVE_ExtensionHeader::GCM_IV_SIZE, nullptr))
        handle_openssl_error("EVP_CTRL_GCM_SET_IVLEN failed");

    // 提供密钥和 IV
    if (1 != EVP_EncryptInit_ex(ctx, nullptr, nullptr, key, iv))
        handle_openssl_error("EVP_EncryptInit_ex failed (key/iv)");

    // 提供 AAD (附加认证数据) - 认证但未加密
    if (!aad.empty()) {
        if (1 != EVP_EncryptUpdate(ctx, nullptr, &len, aad.data(), aad.size()))
            handle_openssl_error("EVP_EncryptUpdate failed (AAD)");
    }

    // 提供明文，获取密文
    ciphertext.resize(plaintext.size());
    if (1 != EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), plaintext.size()))
        handle_openssl_error("EVP_EncryptUpdate failed (Ciphertext)");
    ciphertext_len = len;

    // 结束加密操作
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len))
        handle_openssl_error("EVP_EncryptFinal_ex failed");
    ciphertext_len += len;

    // 设置 Tag 长度并获取 MAC Tag
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag)) // GCM 标签为 16 字节
        handle_openssl_error("EVP_CTRL_GCM_GET_TAG failed");

    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

// -----------------------------------------------------------------
// 4. 主程序和演示
// -----------------------------------------------------------------

//int main() {
//    // 示例常量
//    const size_t KEY_SIZE = 32; // AES-256 Key Size
//    const size_t MAC_TAG_SIZE = 16; // GCM Tag Size
//
//    try {
//        cout << "--- IPv6 EH GCM MAC 模拟程序 ---" << endl;
//
//        // 1. 定义常量 (密钥和明文)
//        // 注意: 实际应用中，密钥 K_E 应通过 ECDH/KDF 派生得到。
//        const uint8_t key_e[KEY_SIZE] = {
//            0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
//            0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
//            0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87,
//            0x98, 0xA9, 0xB0, 0xC1, 0xD2, 0xE3, 0xF4, 0x05
//        };
//        vector<uint8_t> plaintext = {
//            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, // 8 bytes of data
//            0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
//        }; // 16 字节的明文
//
//        // 2. 生成 IV/Nonce
//        uint8_t iv_buffer[HVE_ExtensionHeader::GCM_IV_SIZE];
//        if (RAND_bytes(iv_buffer, sizeof(iv_buffer)) <= 0) handle_openssl_error("RAND_bytes failed");
//
//        // 3. 构造 AAD (附加认证数据)
//        // 实际中 AAD 是 IPv6 基础头和 EH 不变的部分，这里使用一个示例 AAD
//        vector<uint8_t> aad_data = { 0xDE, 0xAD, 0xBE, 0xEF }; // 4 字节的 AAD
//
//        // 检查密文缓冲区大小是否足够
//        if (plaintext.size() > HVE_ExtensionHeader::GCM_CIPHERTEXT_MAX_SIZE) {
//            throw runtime_error("明文过长，无法容纳在 IPv6 扩展头密文主体中。");
//        }
//
//        // 4. 执行 AES-256 GCM 加密和 MAC 生成
//        vector<uint8_t> ciphertext;
//        uint8_t mac_tag[MAC_TAG_SIZE];
//
//        int ciphertext_len = aes_256_gcm_encrypt(
//            plaintext,
//            key_e,
//            iv_buffer,
//            aad_data,
//            ciphertext,
//            mac_tag
//        );
//
//        cout << "\n✅ 加密和 MAC 生成成功." << endl;
//        print_hex("IV/Nonce", iv_buffer, sizeof(iv_buffer));
//        print_hex("AAD (认证数据)", aad_data.data(), aad_data.size());
//        print_hex("密文主体", ciphertext.data(), ciphertext_len);
//        print_hex("GCM MAC 标签", mac_tag, MAC_TAG_SIZE);
//        cout << "实际密文长度: " << ciphertext_len << " 字节" << endl;
//
//
//        // 5. 填充 IPv6 扩展头结构体
//        HVE_ExtensionHeader eh;
//        eh.next_header = 17; // 例如：下一个头部是 UDP
//        eh.eh_length = HVE_ExtensionHeader::EH_LEN_VALUE; // 14, 表示总长度 120 字节
//        eh.r_row_index = 0x01;
//
//        // 5a. 填充 IV
//        copy(iv_buffer, iv_buffer + sizeof(iv_buffer), eh.iv);
//
//        // 5b. 填充密文主体
//        fill(begin(eh.ciphertext_body), end(eh.ciphertext_body), 0x00); // 先清空
//        copy(ciphertext.begin(), ciphertext.end(), eh.ciphertext_body);
//
//        // 5c. 填充 MAC 标签
//        copy(mac_tag, mac_tag + sizeof(mac_tag), eh.gcm_mac_tag);
//
//        // 5d. 填充 32 字节的自动对齐区域
//        fill(begin(eh.padding_32_bytes), end(eh.padding_32_bytes), 0x00);
//
//        cout << "\n--- IPv6 扩展头封装结果 ---" << endl;
//        cout << "扩展头总大小 (Bytes): " << HVE_ExtensionHeader::TOTAL_SIZE << " (115 字节)" << endl;
//        cout << "Next Header: " << (int)eh.next_header << endl;
//        cout << "EH Length (用于网络传输): " << (int)eh.eh_length << endl;
//
//        // 6. 打印封装后的 EH 关键部分
//        print_hex("封装后的 IV", eh.iv, HVE_ExtensionHeader::GCM_IV_SIZE);
//        print_hex("封装后的密文 (前 " + to_string(ciphertext_len) + " 字节)", eh.ciphertext_body, ciphertext_len);
//        print_hex("封装后的 MAC 标签", eh.gcm_mac_tag, MAC_TAG_SIZE);
//
//
//    }
//    catch (const exception& e) {
//        cerr << "\n❌ 发生错误: " << e.what() << endl;
//        return 1;
//    }
//
//    return 0;
//}