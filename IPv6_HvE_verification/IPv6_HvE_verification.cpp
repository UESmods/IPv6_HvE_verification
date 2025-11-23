// IPv6_HvE_verification.cpp: 定义应用程序的入口点。

#include "IPv6_HvE_verification.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

using namespace std;

//文件输出测试函数
int Putdata(vector<uint8_t>& data, const string& filename) {
	// 以二进制写入模式打开文件
	ofstream outfile(filename, ios::binary);
	if (!outfile) {
		cerr << "无法打开文件 \"" << filename << "\" 进行写入。" << endl;
		return 1;
	}

	// 将 vector<uint8_t> 的内容写入文件
	outfile.write(reinterpret_cast<const char*>(data.data()), data.size());

	// 检查写入是否成功
	if (!outfile) {
		cerr << "写入文件 \"" << filename << "\" 时发生错误。" << endl;
		return 1;
	}

	outfile.close();

	cout << "已成功将 " << data.size() << " 字节的数据写入到文件 \"" << filename << "\" 中。" << endl;

	return 0;
}

int main()
{
	//ipv6 openssl初始化检查
	cout << "Starting IPv6_HvE_verification..." << endl;
	const char* version = OpenSSL_version(SSLEAY_VERSION);
	if (version) {
		cout << "OpenSSL successfully linked and initialized." << endl;
		cout << "OpenSSL Version:" << version << endl;
		const char* build_info = OpenSSL_version(OPENSSL_BUILT_ON);
		cout << "OpenSSL Built On:" << (build_info ? build_info : "N/A") << endl;

		SSL_library_init();
		ERR_load_crypto_strings();
		SSL_load_error_strings();
		OpenSSL_add_all_algorithms();

		cout << "OpenSSL libraries initialized." << endl;
	}
	else {
		cerr << "ERROR: Failed to retrieve OpenSSL version. Check library linking." << endl;
	}
	cout << "Verification complete." << endl;

	cout << "2D Data Block Creation Test ---" << endl;

	//网格大小
	const size_t ROWS = 200;
	const size_t COLS = 255;

	//网格生成
	vector<vector<uint8_t>> data_matrix = create_2d_data_block(ROWS, COLS);
	cout << "网格生成函数执行" << endl;
	//随机数生成 随即行ID
	int ASnumber = generate_random_int(1, 199);
	set<size_t> killw = { static_cast<size_t>(ASnumber) };
	cout << "随即行数据ID生成执行" << endl;

	//AES256对称加密
	vector<uint8_t> Key256 = generate_aes_256_key_openssl();
	cout << "AES256密钥生成完毕" << endl;

	vector<uint8_t> KeyIv = generate_secure_iv(12);
	cout << "AES256向量函数生成完毕" << endl;

	//ID行插入
	insert_single_row_at_index(data_matrix, generate_column_marker_row(255), ASnumber);
	cout << "ID行插入函数执行" << endl;

	//SHA-256 哈希值
	vector<uint8_t> Sha256 = calculate_sha256_binary(readFileToBinaryStream("F:/Disciple_rules.txt"));
	cout << "SHA-256哈希生成完成" << endl;

	//二维数据行排除填充\行数随机填充函数
	insert_rows_from_source_exclude_v2(data_matrix, readFileToBinaryStream("F:/Disciple_rules.txt"), 255, killw, 200);
	cout << "二维行数据排除、行数据填充执行" << endl;
	
	//二维乱序函数
	column_permutation(data_matrix);
	cout << "二维乱序列执行" << endl;

	//二维输出 - debug函数可删除
	print_data_block(data_matrix, 15, 20);



	vector<uint8_t> D1data;

	//二维向一维跌落
	D1data = flatten_2d_to_1d(data_matrix);
	cout << "二维向一维跌落函数执行" << endl;
	//数据输出
	Putdata(D1data, "测试1");
	cout << "data output: ";
	for (int i = 0; i < 255 * 10; i++) {
		cout << D1data[i] << " ";
		if (!(i % 255)) cout << "\n-------------------------------------------------------------------------" << endl;
	}



	//封装函数

	//for (auto i: D1data) {
	//	cout << i << " ";
	//}

	return 0;
}
