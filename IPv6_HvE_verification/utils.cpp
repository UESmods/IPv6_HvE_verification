#include "IPv6_HvE_verification.h"

#include <iostream>
#include <sstream>
#include <iomanip>

#include <openssl/sha.h>

/**
 * @brief 计算给定字符串的 SHA-256 哈希值，并输出原始字符串及其哈希值。
 * * @param input 要进行哈希的字符串。
 * @return std::string SHA-256 哈希值的十六进制表示。
 */

using namespace std;

vector<uint8_t> calculate_sha256_binary(const vector<uint8_t>& input) {
    cout << "----------------------------------------" << endl;
    cout << "Input Data (vector<uint8_t]): " << input.size() << " bytes" << endl;

    vector<uint8_t> hash(SHA256_DIGEST_LENGTH); // 32 字节

    SHA256(input.data(), input.size(), hash.data());

    cout << "SHA-256 (binary, 32 bytes) calculated." << endl;
    cout << "----------------------------------------" << endl;

    return hash;
}