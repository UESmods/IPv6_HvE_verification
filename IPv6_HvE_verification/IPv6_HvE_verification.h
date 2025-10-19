// IPv6_HvE_verification.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <set>

// 用于uint8_t 类型
#include <cstdint>

#include <openssl/ssl.h>
#include <openssl/sha.h>
#include <openssl/err.h>
#include <openssl/conf.h>

using namespace std;//声明命名空间

// TODO: 在此处引用程序需要的其他标头。


/**
* @brief 创建数据块 200*255 二维数据块（1/字节每个元素）
* @return vector<vector<uint8_t>> 创建二维数据块使用
*/

vector<vector<uint8_t>> create_2d_data_block(size_t rows, size_t cols);

//二维数据块向一维跌落
vector<uint8_t> flatten_2d_to_1d(const vector<vector<uint8_t>>& data_2d);

//随机整数生成
int generate_random_int(int min_val, int max_val);

//随机数据填充
vector<uint8_t> create_1d_data_row_random(size_t cols_size);

//二维行排除填充
size_t insert_rows_from_source_exclude_v2(
    vector<vector<uint8_t>>& data_block,
    const vector<uint8_t>& source_data,
    size_t cols_per_row,
    const set<size_t>& excluded_rows,
    size_t rows_to_insert);

//ID行插入
bool insert_single_row_at_index(
    vector<vector<uint8_t>>& data_block,
    const vector<uint8_t>& source_row,
    size_t target_index);

//数据文件引入
vector<uint8_t> readFileToBinaryStream(const string& filepath);

//一维列编码函数
vector<uint8_t> generate_column_marker_row(size_t cols_count);

//输出演示模块--可删除
void print_data_block(const vector<vector<uint8_t>>& data_block,
    size_t max_rows_to_print,
    size_t max_cols_to_print);
