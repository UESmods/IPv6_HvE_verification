#include "IPv6_HvE_verification.h"

#include <vector>
#include <cstdint>
#include <iostream>
#include <set>
#include <algorithm>
#include <iomanip>
#include <cmath> 

using namespace std;

// ---------------------- 核心插入函数 V2 (保持不变) ----------------------

/**
 * @brief 使用外部数据源中的数据，以行优先的方式插入或覆盖二维数据块中的行。
 * * 插入从第 0 行开始，跳过排除行索引，并受限于指定的插入行数。
 *
 * @param data_block 要插入/覆盖行的二维数据块的引用。
 * @param source_data 包含用于插入的数据的一维数组。
 * @param cols_per_row 每行应有的列数（即 source_data 中多少字节构成一行）。
 * @param excluded_rows 包含不应被插入或覆盖的行索引的集合（这些索引位置将被跳过）。
 * @param rows_to_insert 期望从 source_data 中提取并插入的总行数（即使是部分行）。
 * @return size_t 实际插入或覆盖的行数。
 */
size_t insert_rows_from_source_exclude_v2(
    vector<vector<uint8_t>>& data_block,
    const vector<uint8_t>& source_data,
    size_t cols_per_row,
    const set<size_t>& excluded_rows,
    size_t rows_to_insert) {

    // 1. 基本参数校验
    if (cols_per_row == 0) {
        cerr << "Error: Columns per row (cols_per_row) cannot be zero." << endl;
        return 0;
    }
    if (source_data.empty() || rows_to_insert == 0) {
        cout << "Warning: Source data is empty or rows_to_insert is zero. No rows inserted." << endl;
        return 0;
    }

    // 2. 准备插入操作的变量
    size_t source_index = 0;
    size_t inserted_rows_count = 0; // 实际插入/覆盖的行数
    size_t source_rows_processed = 0; // 已从 source_data 中提取的行数（包括部分行）
    size_t target_row_index = 0;     // 当前在 data_block 中尝试插入的目标位置

    cout << "Attempting to insert up to " << rows_to_insert
        << " source rows, scanning from target index 0 (Cols per row: " << cols_per_row << ")." << endl;

    // 3. 循环直到达到期望的插入行数，或者用完了源数据
    while (source_rows_processed < rows_to_insert && source_index < source_data.size()) {

         //A. 检查当前的目标索引是否在排除列表中
        if (excluded_rows.count(target_row_index)) {
            cout << "Skipping insertion at target index [" << target_row_index
                << "] as it is explicitly excluded. Source data is NOT consumed." << endl;
            // 跳过排除的行索引，继续检查下一个目标索引
            target_row_index++;
            continue; // 跳到下一次 while 循环，不处理源数据行
        }

        // B. 检查源数据是否足以构成一行 (如果不足，则构成部分行)
        size_t current_row_len = min(cols_per_row, source_data.size() - source_index);

        // C. 从源数据中提取行数据
        vector<uint8_t> new_row;
        // 使用 range insert 提取当前行的数据
        new_row.insert(new_row.end(),
            source_data.begin() + source_index,
            source_data.begin() + source_index + current_row_len);

        // D. 插入或覆盖到数据块中
        if (target_row_index < data_block.size()) {
            // 如果目标位置已存在，则进行覆盖
            data_block[target_row_index] = new_row;
            // 提示用户哪一行被覆盖
            if (target_row_index < 5 || target_row_index > data_block.size() - 5) {
                cout << "Overwriting row [" << target_row_index << "] with " << new_row.size() << " bytes (Source row " << source_rows_processed << ")." << endl;
            }
            else if (target_row_index == 5) {
                cout << "..." << endl;
            }
        }
        else {
            // 如果目标位置超过了当前块大小，则追加新行
            data_block.push_back(new_row);
            // 提示用户哪一行被追加
            if (target_row_index > data_block.size() - 5) {
                cout << "Appending new row [" << target_row_index << "] with " << new_row.size() << " bytes (Source row " << source_rows_processed << ")." << endl;
            }
        }

        // E. 更新计数器
        source_index += current_row_len;      // 源数据索引前进
        inserted_rows_count++;                // 实际操作的行数增加
        source_rows_processed++;              // 已处理的源数据行数增加
        target_row_index++;                   // 移动到下一个目标插入位置
    }

    cout << "Total rows in data block is now " << data_block.size() << "." << endl;
    cout << "Successfully inserted/overwritten " << inserted_rows_count << " rows." << endl;

    return inserted_rows_count;
}