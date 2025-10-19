#include "utils.h"
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

string calulate_and_print_sha256(const string& input) {
	cout << "----------------------------------------" << endl;
	cout << "Input String: \"" << input << "\"" << endl;

	unsigned char hash[SHA256_DIGEST_LENGTH];

	if (input.empty()) {
		SHA256(nullptr, 0, hash);
	}
	else {
		SHA256(
			reinterpret_cast<const unsigned char*>(input.c_str()),
			input.length(),
			hash
		);
	}
	//cout << endl;
	//cout << "Binary_data:" << hash << endl;

	stringstream ss;
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
		ss << hex << setw(2) << setfill('0') << static_cast<int>(hash[i]);
	}
	string hash_value = ss.str();

	cout << "SHA-256 Hash:" << hash_value << endl;
	cout << "----------------------------------------" << endl;
	
	return hash_value;
}