#include "IPv6_HvE_verification.h"

#include <vector>
#include <cstdint>
#include <iostream>

using namespace std;

/**
 * @brief 将一个二维数据块 (vector<vector<uint8_t>>) 转换为一个一维数组 (vector<uint8_t>)，按行优先顺序。
 * * @param data_2d 要展平的二维数据块
 * @return vector<uint8_t> 展平后的一维数据块
 */
vector<uint8_t> flatten_2d_to_1d(const vector<vector<uint8_t>>& data_2d) {
    if (data_2d.empty()) {
        return vector<uint8_t>(); // 返回空的一维数组
    }

    // 预先计算总大小，以优化内存分配和性能
    size_t total_size = 0;
    for (const auto& row : data_2d) {
        total_size += row.size();
    }

    // 初始化一维数组并预分配内存
    vector<uint8_t> data_1d;
    data_1d.reserve(total_size);

    // 按行优先顺序进行跌落/展平
    // 外层循环遍历每一行
    for (const auto& row : data_2d) {
        // 内层操作将整行元素追加到 data_1d 的末尾
        // 使用 insert 可以高效地将另一个 vector 的内容追加进来
        data_1d.insert(data_1d.end(), row.begin(), row.end());
    }

    // 另一种更简单的基于循环的实现方式 :
    /*
    for (const auto& row : data_2d) {
        for (uint8_t element : row) {
            data_1d.push_back(element);
        }
    }
    */

    cout << "2D data block successfully flattened to 1D array." << endl;
    cout << "Total 1D elements: " << data_1d.size() << endl;

    return data_1d;
}