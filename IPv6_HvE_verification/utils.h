#pragma once

#include <string>

using namespace std;

/**
 * @brief 计算给定字符串的 SHA-256 哈希值，并输出原始字符串及其哈希值。
 * * @param input 要进行哈希的字符串。
 * @return string SHA-256 哈希值的十六进制表示。
 */
string calulate_and_print_sha256(const string& input);