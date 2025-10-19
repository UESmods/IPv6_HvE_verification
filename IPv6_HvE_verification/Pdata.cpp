
#include "IPv6_HvE_verification.h"

#include <iostream>
#include <vector>
#include <cstdint>
#include <iomanip>
#include <algorithm> // 包含 min

using namespace std; // 使用命名空间 std

/**
 * @brief 打印二维数据块中的所有数据，将其 uint8_t 元素以十六进制格式输出。
 * 并限制打印范围以避免输出过多。
 * @param data_block 要打印的二维数据块
 * @param max_rows_to_print 允许打印的最大行数 (默认 10)
 * @param max_cols_to_print 允许打印的最大列数 (默认 16)
 */
void print_data_block(const vector<vector<uint8_t>>& data_block,
    size_t max_rows_to_print = 10,
    size_t max_cols_to_print = 16) {
    if (data_block.empty()) {
        cout << "Data block is empty." << endl;
        return;
    }

    size_t rows = data_block.size();
    size_t cols = data_block[0].size();

    // 确定实际打印的行数和列数
    size_t rows_to_print = min(rows, max_rows_to_print);
    size_t cols_to_print = min(cols, max_cols_to_print);

    cout << "\n--- Data Block Content (" << rows_to_print << "x" << cols_to_print << " partial view) ---" << endl;
    cout << "Element type: uint8_t (HEX value)" << endl;

    // 打印列索引 (头部)
    cout << "Row\\Col |";
    for (size_t j = 0; j < cols_to_print; ++j) {
        // 使用 setw(4) 确保对齐
        cout << setw(4) << j;
    }
    if (cols > cols_to_print) {
        cout << " ..."; // 如果有更多列，打印省略号
    }
    cout << endl;

    // 打印分隔线
    cout << "-------+";
    for (size_t j = 0; j < cols_to_print; ++j) {
        cout << "----";
    }
    if (cols > cols_to_print) {
        cout << "----";
    }
    cout << endl;

    // 设置全局输出为十六进制，并保持大写和填充
    cout << hex << uppercase << setfill('0');

    // 打印数据主体
    for (size_t i = 0; i < rows_to_print; ++i) {
        // 打印行索引 (需转回十进制)
        // 注意：这里需要先重置格式，打印索引后再设置回来
        cout << dec << setw(6) << i << " |";

        // 重新设置十六进制格式
        cout << hex;

        // 打印列数据
        for (size_t j = 0; j < cols_to_print; ++j) {
            // 将 uint8_t 转换为 int 以打印其数值
            // setw(2) 表示输出宽度至少为 2，setfill('0') 保证不足两位时用 0 填充
            cout << setw(2) << static_cast<int>(data_block[i][j]) << "  "; // 2位HEX + 2个空格 = 4个字符宽度
        }

        // 如果列数被截断，打印省略号
        if (cols > cols_to_print) {
            cout << "...";
        }
        cout << endl;
    }

    // 如果行数被截断，打印省略号
    if (rows > rows_to_print) {
        // 确保行索引的省略号使用十进制格式
        cout << dec << setw(6) << "..." << " |";

        // 重新设置十六进制格式，以便打印内容对齐
        cout << hex;

        // 确保省略号与数据对齐
        for (size_t j = 0; j < cols_to_print; ++j) {
            // ".." + "  " = 4个字符宽度，与数据对齐
            cout << ".." << "  ";
        }
        if (cols > cols_to_print) {
            cout << "...";
        }
        cout << endl;
    }

    // 恢复默认的输出格式（重要！）
    cout << dec << setfill(' ');
    cout << "---------------------------------------------------" << endl;
}