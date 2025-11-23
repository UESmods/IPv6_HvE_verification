#include "IPv6_HvE_verification.h"

#include <vector>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <stdexcept> // 包含 std::out_of_range

using namespace std;

// ---------------------- 核心函数 ----------------------

/**
 * @brief 生成一个一维数据行，其中的每个元素代表一个递增的列标记。
 * 标记从 0 开始递增，每个元素占用一个字节 (uint8_t)。
 *
 * @param cols_count 要生成的列标记数量。
 * @return vector<uint8_t> 包含列标记的一维数组。
 */
vector<uint8_t> generate_column_marker_row(size_t cols_count) {
    // 检查列数是否超出 uint8_t 的编码范围 (0-255)
    if (cols_count > 256) {
        // 如果需要生成超过 256 个标记，应该使用更大的类型 (如 uint16_t)，或者修改编码方式。
        throw out_of_range("Error: cols_count exceeds the range of uint8_t (max 256 markers: 0-255).");
    }

    // 初始化一维数组并预分配内存
    vector<uint8_t> marker_row;
    marker_row.reserve(cols_count);

    // 从 0 开始，逐一生成列标记
    for (size_t j = 0; j < cols_count; ++j) {
        // 将 size_t 类型的索引 j 转换为 uint8_t
        // 这里的转换是安全的，因为我们在函数开头检查了 cols_count <= 256
        uint8_t marker_value = static_cast<uint8_t>(j);
        marker_row.push_back(marker_value);
    }

    cout << "Successfully generated a 1D column marker row with " << cols_count << " elements." << endl;

    return marker_row;
}