#include "IPv6_HvE_verification.h"

#include <vector>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <string>

using namespace std;

// ---------------------- 核心函数：单行指定索引插入 ----------------------

/**
 * @brief 在二维数据块中指定的 target_index 处，插入或覆盖单个 1D 数据行。
 *
 * @param data_block 要修改的二维数据块的引用。
 * @param source_row 包含用于插入的单个行数据（1D 数组）。
 * @param target_index 要插入或覆盖的目标行索引。
 * @param cols_per_row 期望的列数（用于校验）。
 * @return bool 插入或覆盖是否成功。
 */
 // ---------------------- 核心函数：单行指定索引插入 (已修改：移除列数校验) ----------------------

 /**
  * @brief 在二维数据块中指定的 target_index 处，插入或覆盖单个 1D 数据行。
  * 直接用 source_row 的内容和大小覆盖目标行，不进行列数校验。
  *
  * @param data_block 要修改的二维数据块的引用。
  * @param source_row 包含用于插入的单个行数据（1D 数组）。
  * @param target_index 要插入或覆盖的目标行索引。
  * @return bool 插入或覆盖是否成功。
  */
bool insert_single_row_at_index(
    vector<vector<uint8_t>>& data_block,
    const vector<uint8_t>& source_row,
    size_t target_index) { // 移除了 cols_per_row 参数

    // 1. 处理插入/覆盖逻辑
    if (target_index < data_block.size()) {
        // 目标索引在现有范围内：覆盖 (Overwrite)
        // 直接赋值，source_row 的大小将成为 data_block[target_index] 的新大小
        data_block[target_index] = source_row;
        cout << "[成功] 覆盖现有行索引: " << target_index << " (新行大小: " << source_row.size() << ")。" << endl;
        return true;
    }
    else if (target_index == data_block.size()) {
        // 目标索引刚好是末尾：追加 (Append)
        data_block.push_back(source_row);
        cout << "[成功] 在末尾追加新行，索引: " << target_index << " (新行大小: " << source_row.size() << ")。" << endl;
        return true;
    }
    else {
        // 目标索引远远大于末尾：无法进行稀疏插入
        cerr << "[失败] 目标索引 (" << target_index << ") 超过了当前数据块末尾 ("
            << data_block.size() << ")，无法进行非连续追加。" << endl;
        return false;
    }
}