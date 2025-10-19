#include "IPv6_HvE_verification.h"

#include <vector>
#include <cstdint>
#include <iostream>
#include <random>
#include <chrono>
#include <iomanip> // 用于演示输出

using namespace std;

/**
 * @brief 创建一个指定大小的一维数据行 (vector<uint8_t>)，并用随机数据填充。
 * @param cols_size 要创建的一维数据行的元素数量 (列数)
 * @return vector<uint8_t> 创建并随机填充的一维数据行
 */
vector<uint8_t> create_1d_data_row_random(size_t cols_size) {
    // 1. 设置随机数生成器
    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    default_random_engine generator(seed);

    // 创建一个从 0 到 255 的均匀整数分布 (对应 uint8_t 范围)
    uniform_int_distribution<int> distribution(0, 255);

    // 2. 初始化一维数据行并预分配内存
    // 创建一个包含 cols_size 个元素的 vector，初始值可忽略
    vector<uint8_t> data_row(cols_size);

    // 3. 随机填充
    for (size_t j = 0; j < cols_size; ++j) {
        // 生成一个 0 到 255 的随机整数，并转换为 uint8_t
        data_row[j] = static_cast<uint8_t>(distribution(generator));
    }

    cout << "Successfully created a 1D data row with " << cols_size << " elements." << endl;

    return data_row;
}