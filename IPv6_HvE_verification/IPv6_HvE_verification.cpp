// IPv6_HvE_verification.cpp: 定义应用程序的入口点。

#include "IPv6_HvE_verification.h"
#include "utils.h"

using namespace std;

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

	cout << "Starting IPv6_HvE_verification with SHA-256 test..." << endl;

	string data1 = "Hellow World";
	string data2 = "The quick brown fox jumps over the lazy dog";
	string data3 = "";

	calulate_and_print_sha256(data1);
	calulate_and_print_sha256(data2);
	calulate_and_print_sha256(data3);

	cout << "2D Data Block Creation Test ---" << endl;
	
	//网格大小
	const size_t ROWS = 200;
	const size_t COLS = 255;

	//网格生成
	vector<vector<uint8_t>> data_matrix = create_2d_data_block(ROWS, COLS);

	//随机数生成
	int ASnumber = generate_random_int(1, 199);
	set<size_t> killw = { static_cast<size_t>(ASnumber) };

	//ID行插入
	insert_single_row_at_index(data_matrix, generate_column_marker_row(255), ASnumber);

	//二维数据行排除填充\行数随机填充函数
	insert_rows_from_source_exclude_v2(data_matrix, readFileToBinaryStream("C:/Users/Public/Desktop/Steam.lnk"), 255, killw, 200);
	
	//二维输出
	print_data_block(data_matrix, 200, 20);

	vector<uint8_t> D1data;

	//二维向一维跌落
	D1data = flatten_2d_to_1d(data_matrix);

	//数据输出
	cout << "data output: ";
	for (int i = 0; i < 255 * 10; i++) {
		cout << D1data[i] << " ";
		if (!(i % 255)) cout << "\n-------------------------------------------------------------------------" << endl;
	}
	//for (auto i: D1data) {
	//	cout << i << " ";
	//}

	return 0;
}
