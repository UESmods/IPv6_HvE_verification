#include "IPv6_HvE_verification.h"

/**
* IPv6扩展头
*/

#include <cstdint>
#include <vector>
#include <iostream>

// 使用标准命名空间
using namespace std;

/**
 * @brief HVE (Host-Verified Encryption) IPv6 扩展头结构体定义。
 * * 扩展头的总长度应为 8 字节的倍数，而您要求的总长度是：
 * 1 (下一头部) + 1 (EH长度) + 1 (R行索引) + 64 (密文/IV) + 16 (MAC标签) + 32 (自动对齐/填充)
 * 总计: 1 + 1 + 1 + 64 + 16 + 32 = 115 字节。
 * * EH长度字段 (EH Len) 的计算方式是：
 * EH长度 = (整个EH长度 / 8) - 1。
 * * 因为 115 字节不是 8 的倍数，所以最终 EH 长度字段的值需要根据实际对齐后的长度来定。
 * 我们将结构体对齐到 115 字节，并通过 32 字节的填充来保证整体长度。
 */

 // 启用 1 字节对齐，以确保字段之间没有编译器自动插入的填充
#pragma pack(push, 1)

struct HVE_ExtensionHeader {
    // --- 必选头部字段 (3 字节) ---

    /**
     * @brief 下一头部 (Next Header)
     * 指示紧随此扩展头之后的头部类型。
     * 例如，下一个是 TCP (6) 或 UDP (17)。
     */
    uint8_t next_header;

    /**
     * @brief EH 长度 (Header Extension Length)
     * 整个扩展头（不包括前 8 字节）的长度除以 8，再减 1。
     * * 计算: 整个EH长度 = 1 + 1 + 1 + 64 + 16 + 32 = 115 字节。
     * 由于 IPv6 EH 必须是 8 字节的倍数，如果要求 32 字节填充，实际传输长度可能需要四舍五入到 120 字节。
     * 如果严格遵循您的定义 (115 字节)，则 EH 长度字段的值是不标准的。
     * 假设我们遵循您的字段大小要求，并在末尾添加填充以达到 120 字节的 8 倍数对齐，
     * 则总长度为 120 字节。EH长度字段的值为 (120 / 8) - 1 = 14。
     */
    uint8_t eh_length;

    /**
     * @brief R 行索引 (Row Index)
     * 用于指示 ECC 或 HVE 方案中的特定行或索引。
     */
    uint8_t r_row_index;

    // --- 加密和认证字段 (64 + 16 = 80 字节) ---

    /**
     * @brief ECC 密文 + IV (64 字节)
     * 包含加密后的数据和初始化向量 (IV/Nonce)。
     * 假设 IV/Nonce 包含在 ECC 密文中。
     * * 实际 ECC 密文通常包含：临时公钥 + 密文主体。
     * 这里的 64 字节是固定长度。
     */
    uint8_t ecc_ciphertext_iv[64];

    /**
     * @brief GCM MAC 标签 (16 字节)
     * 用于数据的完整性和认证。
     */
    uint8_t gcm_mac_tag[16];

    // --- 对齐字段 (32 字节) ---

    /**
     * @brief 自动对齐填充 (32 字节)
     * 结构体的总大小为 1 + 1 + 1 + 64 + 16 + 32 = 115 字节。
     * 这个字段用于填充，以达到您指定的对齐要求。
     */
    uint8_t padding_32_bytes[32];

    // --- 辅助方法 ---

    /**
     * @brief 获取此扩展头的总字节数。
     * @return 结构体的总大小，即 115 字节。
     */
    size_t get_total_length() const {
        return sizeof(HVE_ExtensionHeader);
    }
};

// 恢复默认的字节对齐设置
#pragma pack(pop)

/**
 * @brief 演示函数：打印扩展头信息。
 */
void print_eh_info(const HVE_ExtensionHeader& eh) {
    cout << "--- HVE 扩展头信息 ---" << endl;
    cout << "结构体总大小 (Bytes): " << eh.get_total_length() << endl;

    // IPv6 扩展头要求总长度是 8 字节的倍数。
    // 115 字节不满足，下一个 8 的倍数是 120。
    // 您的 eh_length 字段值应该基于 120 字节 (120/8 - 1 = 14)。
    cout << "IPv6 EH 长度字段 EH_Len: " << (int)eh.eh_length << " (应为 14, 表示 120 字节)" << endl;
    cout << "下一头部 Next Header: " << (int)eh.next_header << endl;
    cout << "R 行索引 Row Index: " << (int)eh.r_row_index << endl;
    cout << "ECC密文+IV 长度: " << sizeof(eh.ecc_ciphertext_iv) << " 字节" << endl;
    cout << "GCM MAC标签长度: " << sizeof(eh.gcm_mac_tag) << " 字节" << endl;
    cout << "填充 (Padding) 长度: " << sizeof(eh.padding_32_bytes) << " 字节" << endl;
}

/**
 * @brief 主演示函数
 */
//int main() {
//    // 实例化扩展头
//    HVE_ExtensionHeader eh;
//
//    // 填充示例数据
//    eh.next_header = 6; // 例如，下一个头部是 TCP
//    // 根据 115 字节的总长度，实际需要的 EH 字段长度是 (120/8) - 1 = 14
//    eh.eh_length = 14;
//    eh.r_row_index = 0xAF;
//
//    // 填充密文和 MAC 标签（这里用 0x00 填充，实际应为加密数据）
//    fill(begin(eh.ecc_ciphertext_iv), end(eh.ecc_ciphertext_iv), 0xAA);
//    fill(begin(eh.gcm_mac_tag), end(eh.gcm_mac_tag), 0xBB);
//    fill(begin(eh.padding_32_bytes), end(eh.padding_32_bytes), 0x00);
//
//    // 打印信息
//    print_eh_info(eh);
//
//    // 验证结构体大小是否符合预期 (115 字节)
//    if (sizeof(HVE_ExtensionHeader) == 115) {
//        cout << "\n✅ 结构体大小验证成功: sizeof(HVE_ExtensionHeader) == 115 字节" << endl;
//    }
//    else {
//        cout << "\n❌ 结构体大小验证失败: sizeof(HVE_ExtensionHeader) == " << sizeof(HVE_ExtensionHeader) << " 字节" << endl;
//    }
//
//    return 0;
//}