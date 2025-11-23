#include "IPv6_HvE_verification.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm> // 用于 min
#include <cstdio>    // 用于 remove
#include <cstdint>   // 新增：用于 uint8_t

using namespace std;

/**
 * @brief 将指定文件读取为二进制数据流，最大限制为 51000 字节。
 * 返回类型已修改为 vector<uint8_t> 以解决外部函数的类型匹配问题。
 *
 * @param filepath 要读取的文件的路径。
 * @return vector<uint8_t> 包含文件内容的二进制数据流（1字节无符号整数）。
 * 如果文件不存在或读取失败，则返回空向量。
 */
vector<uint8_t> readFileToBinaryStream(const string& filepath) {
    // 定义最大的字节限制
    const size_t MAX_SIZE = 51000;

    // 1. 打开文件。使用 ios::binary 进行二进制读取，
    //    使用 ios::ate 将文件指针定位到文件末尾，以便获取文件大小。
    ifstream file(filepath, ios::binary | ios::in | ios::ate);

    // 检查文件是否成功打开
    if (!file.is_open()) {
        cerr << "错误：无法打开文件 " << filepath << endl;
        return {}; // 返回空向量表示失败
    }

    // 2. 获取文件大小
    size_t fileSize = static_cast<size_t>(file.tellg());

    // 3. 计算实际要读取的大小，取文件大小和最大限制中的较小值
    size_t sizeToRead = min(fileSize, MAX_SIZE);

    // 4. 将文件指针重置回文件开头
    file.seekg(0, ios::beg);

    // 5. 创建一个足够大的 vector<uint8_t> 来存储数据
    vector<uint8_t> buffer(sizeToRead);

    // 6. 读取数据
    if (sizeToRead > 0) {
        // ifstream::read 需要 char*，所以需要进行类型转换
        file.read(reinterpret_cast<char*>(buffer.data()), sizeToRead);

        // 检查读取操作是否成功
        if (!file) {
            cerr << "警告：文件读取操作可能未完成（例如，在文件末尾）" << endl;
        }
    }

    // 7. 关闭文件并返回数据
    file.close();

    cout << "文件路径: " << filepath << endl;
    cout << "原始文件大小: " << fileSize << " 字节" << endl;
    cout << "最大允许大小: " << MAX_SIZE << " 字节" << endl;
    cout << "实际读取大小: " << buffer.size() << " 字节" << endl;

    return buffer;
}