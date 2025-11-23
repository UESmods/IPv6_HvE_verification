#include "IPv6_HvE_verification.h"

/**
* 列数据排列
*/

#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <numeric>
#include <stdexcept> // 用于异常处理
#include <utility>   // 用于 std::pair
#include <cmath>

// 使用 using namespace std;
using namespace std;

/**
 * @brief 对一个二维向量（矩阵）进行列随机置换。
 *
 * 所有行的排列顺序都一样，即为列置换。
 *
 * @param matrix 待处理的二维向量。该函数会修改此向量。
 * @return vector<size_t> 返回用于置换的列索引映射 (permutation map)。
 */
vector<size_t> column_permutation(vector<vector<uint8_t>>& matrix) {
    if (matrix.empty()) {
        return {};
    }

    size_t num_rows = matrix.size();

    // 1. **动态调整：计算最大列数 (Max Columns)**
    size_t max_cols = 0;
    for (const auto& row : matrix) {
        max_cols = max(max_cols, row.size());
    }

    // 如果所有行都是空的，则返回
    if (max_cols == 0) {
        return {};
    }

    // 将 max_cols 设置为统一的列数
    size_t num_cols = max_cols;

    // 2. **动态调整：填充 (Padding) 较短的行**
    // 确保所有行的长度都等于 num_cols
    for (size_t i = 0; i < num_rows; ++i) {
        if (matrix[i].size() < num_cols) {
            // 填充到 num_cols 长度，用 0 (uint8_t) 填充
            // 注意：选择 0 作为填充值是基于常见的处理方式。
            // 如果您的业务逻辑对填充值有特殊要求，请更改此处的 0。
            matrix[i].resize(num_cols, 0);
        }
    }
    // 至此，matrix 已经是一个矩形矩阵 (num_rows x num_cols)

    // 3. 创建并初始化列索引向量 (0, 1, 2, ..., N-1)
    vector<size_t> p_map(num_cols);
    iota(p_map.begin(), p_map.end(), 0);

    // 4. 初始化随机数生成器并随机置换索引
    random_device rd;
    mt19937 generator(rd());
    shuffle(p_map.begin(), p_map.end(), generator);
    // 此时 p_map 存储了置换后的列顺序

    // 5. 应用置换到矩阵 (使用 new_matrix 避免内存溢出风险)
    vector<vector<uint8_t>> new_matrix(num_rows, vector<uint8_t>(num_cols));

    for (size_t row = 0; row < num_rows; ++row) {
        for (size_t new_col = 0; new_col < num_cols; ++new_col) {
            size_t old_col = p_map[new_col];
            // 访问是安全的，因为 matrix 已经被填充成矩形
            new_matrix[row][new_col] = matrix[row][old_col];
        }
    }

    // 6. 使用 std::swap 交换 (O(1)操作)
    matrix.swap(new_matrix);

    return p_map;
}

/**
 * @brief 还原列置换，根据参考行中 uint8_t 的值从小到大排序。
 *
 * @param matrix 待还原的二维向量。该函数会修改此向量。
 * @param reference_row 置换前的原始行数据。用于确定还原后的正确列顺序。
 */
void undo_column_permutation(
    vector<vector<uint8_t>>& matrix,
    const vector<uint8_t>& reference_row)
{
    if (matrix.empty() || matrix[0].empty()) {
        return;
    }

    size_t num_rows = matrix.size();
    size_t num_cols = matrix[0].size(); // 此时 num_cols 是填充后的统一列数

    // 检查参考行和矩阵的列数是否匹配 (如果 reference_row 长度小于 num_cols，这里会抛出异常)
    if (reference_row.size() != num_cols) {
        // 如果 matrix 在 column_permutation 中被填充了，reference_row 应该与填充后的列数匹配。
        // 如果您的业务允许 reference_row 也被填充，则需要在外部或此处对 reference_row 进行 resize。
        throw runtime_error("Reference row size does not match matrix column count. Check if reference_row needs padding.");
    }

    // 以下是上次修复后的逻辑，用于处理重复值和内存优化，保持不变。

    // 1. 确定目标顺序：创建 {值, 原始索引} 的结构
    vector<pair<uint8_t, size_t>> sorted_ref(num_cols);
    for (size_t i = 0; i < num_cols; ++i) {
        sorted_ref[i] = { reference_row[i], i };
    }

    // 2. 稳定排序：按值排序，值相同时按原始索引排序
    sort(sorted_ref.begin(), sorted_ref.end(),
        [](const auto& a, const auto& b) {
            if (a.first != b.first) {
                return a.first < b.first;
            }
            return a.second < b.second;
        });

    // 3. 构建还原映射 (restore_map)
    vector<uint8_t> temp_permuted_row = matrix[0];
    vector<size_t> restore_map(num_cols);

    for (size_t target_col = 0; target_col < num_cols; ++target_col) {
        uint8_t target_value = sorted_ref[target_col].first;
        bool found = false;

        for (size_t current_col = 0; current_col < num_cols; ++current_col) {
            if (temp_permuted_row[current_col] == target_value) {
                restore_map[target_col] = current_col;
                found = true;
                temp_permuted_row[current_col] = 0xFF; // 标记已使用
                break;
            }
        }

        if (!found) {
            throw runtime_error("Value for restoration not found in the permuted matrix row. Data mismatch suspected.");
        }
    }

    // 4. 应用还原置换到矩阵 (使用 swap 优化内存)
    vector<vector<uint8_t>> new_matrix(num_rows, vector<uint8_t>(num_cols));

    for (size_t row = 0; row < num_rows; ++row) {
        for (size_t target_col = 0; target_col < num_cols; ++target_col) {
            size_t current_col = restore_map[target_col];
            new_matrix[row][target_col] = matrix[row][current_col];
        }
    }

    matrix.swap(new_matrix);
}

// --- 示例代码 (main 函数) ---
void print_matrix(const vector<vector<uint8_t>>& matrix, const string& title) {
    cout << "--- " << title << " ---" << endl;
    if (matrix.empty()) {
        cout << "[Empty]" << endl;
        return;
    }
    for (const auto& row : matrix) {
        for (uint8_t val : row) {
            // 将 uint8_t 转换为 int 打印数值
            cout << (int)val << "\t";
        }
        cout << endl;
    }
}

//int main() {
//    try {
//        // 原始矩阵：列按照第一行值 (10, 20, 30, 40) 的顺序排列
//        vector<vector<uint8_t>> original_matrix = {
//            {10, 20, 30, 40}, // Reference Row
//            {11, 21, 31, 41},
//            {12, 22, 32, 42}
//        };
//
//        // 保存原始参考行数据
//        const vector<uint8_t> reference_row = original_matrix[0];
//
//        print_matrix(original_matrix, "1. 原始矩阵");
//
//        // ----------------------------------------------------
//        // 步骤 A: 置换 (Permutation)
//        // ----------------------------------------------------
//        vector<size_t> p_map = column_permutation(original_matrix);
//
//        cout << "\n置换映射 (p_map): {";
//        for (size_t i = 0; i < p_map.size(); ++i) {
//            cout << p_map[i] << (i == p_map.size() - 1 ? "" : ", ");
//        }
//        cout << "}" << endl;
//
//        print_matrix(original_matrix, "2. 置换后的矩阵");
//
//        // ----------------------------------------------------
//        // 步骤 B: 还原 (Reversal)
//        // ----------------------------------------------------
//        undo_column_permutation(original_matrix, reference_row);
//
//        print_matrix(original_matrix, "3. 还原后的矩阵 (应与原始矩阵相同)");
//
//    }
//    catch (const exception& e) {
//        cerr << "Exception: " << e.what() << endl;
//        return 1;
//    }
//
//    return 0;
//}