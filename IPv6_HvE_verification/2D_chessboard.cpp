#include <iostream>
#include <stdexcept>
#include <iomanip>
#include <vector>
#include <cstdint>
#include <cmath>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/conf.h>

#include "IPv6_HvE_verification.h"

using namespace std;


/**
 * @brief 创建一个 200x255 的二维数据块 (1 字节/元素)
 * @param rows 块的行数
 * @param cols 块的列数
 * @return std::vector<std::vector<uint8_t>> 创建的二维数据块
 */
vector<vector<uint8_t>> create_2d_data_block(size_t rows, size_t cols) {
	// 使用 std::vector<std::vector<uint8_t>> 初始化一个 R行 C列的二维数组
	// 外部 vector 表示行数 (rows)
	// 内部 vector 表示列数 (cols)，并初始化所有元素为 0
	vector<vector<uint8_t>> data_block(rows, vector<uint8_t>(cols, 0));
    // 示例填充：将每个元素设置为 (行索引 + 列索引) 的结果，取模 256
   // 这样数据块中就会包含 0 到 255 的随机分布值
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            //data_block[i][j] = static_cast<uint8_t>((i + j) % 256);
            data_block[i][j] = 0;
        }
    }

    // 计算总内存使用量 (仅用于演示)
    size_t total_elements = rows * cols;
    size_t memory_bytes = total_elements * sizeof(uint8_t);

    std::cout << "Successfully created " << rows << "x" << cols
        << " 2D data block." << std::endl;
    std::cout << "Total elements: " << total_elements << std::endl;
    std::cout << "Memory occupied: " << memory_bytes << " bytes ("
        << std::fixed << std::setprecision(2) << static_cast<double>(memory_bytes) / 1024.0 << " KB)" << std::endl;

    // 打印几个角上的元素进行验证
    std::cout << "Verification of corner elements:" << std::endl;
    std::cout << "  [0][0]: " << static_cast<int>(data_block[0][0]) << std::endl; // 0
    std::cout << "  [0][254]: " << static_cast<int>(data_block[0][cols - 1]) << std::endl; // 254
    std::cout << "  [199][0]: " << static_cast<int>(data_block[rows - 1][0]) << std::endl; // 199
    std::cout << "  [199][254]: " << static_cast<int>(data_block[rows - 1][cols - 1]) << std::endl; // (199+254) % 256 = 453 % 256 = 197

    return data_block;
}