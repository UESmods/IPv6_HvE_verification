#include "IPv6_HvE_verification.h"

#include <iostream>
#include <cstdint>
#include <array>
#include <vector>
#include <memory>
#include <stdexcept>
#include <algorithm> // 引入 std::copy

// 使用标准命名空间，简化代码书写
using namespace std;

/**
 * 包封装函数
 * 将 iPv6 的扩展头完善后在来写这个
 * */

 // --- 平台相关的打包宏和字节序转换 ---

 // 假设我们使用 GCC/Clang 的环境
#if defined(__GNUC__) || defined(__clang__)
#define PACKED __attribute__((packed))
#else
// 对于其他编译器，可能需要使用 #pragma pack(1)
#define PACKED
#endif

// 假设主机是小端序 (Little-Endian)，需要转换为大端序 (Big-Endian)
// 警告：这些函数是模拟的，在跨平台环境中需要使用系统级的 htons/htonl 或明确的字节序检测。
inline uint16_t HostToNet16(uint16_t h) {
    // 假设是小端序，需要翻转
    return (h << 8) | (h >> 8);
}
inline uint32_t HostToNet32(uint32_t h) {
    // 假设是小端序，需要翻转
    return ((h & 0xFF) << 24) | ((h & 0xFF00) << 8) | ((h & 0xFF0000) >> 8) | (h >> 24);
}

// --- 核心结构体定义 ---

/**
 * @brief IPv6 基本头结构体
 * * 使用 PACKED 确保结构体大小严格为 40 字节。
 * * 所有多字节字段在赋值时都应转换为网络字节序。
 */
struct Ipv6BaseHeader PACKED {
    // 字段 1: Version, Traffic Class, Flow Label (32 bits)
    // 存储在网络字节序中
    uint32_t v_tc_flow;

    // 字段 2: Payload Length (16 bits)
    // 负载的长度（不包括基本头），网络字节序
    uint16_t payload_length;

    // 字段 3: Next Header (8 bits)
    // 指示下一个头部的类型
    uint8_t next_header;

    // 字段 4: Hop Limit (8 bits)
    // 跳数限制
    uint8_t hop_limit;

    // 字段 5: Source Address (128 bits)
    array<uint8_t, 16> source_address;

    // 字段 6: Destination Address (128 bits)
    array<uint8_t, 16> destination_address;

    /**
     * @brief 初始化 IPv6 基本头字段。
     * * 负责将主机字节序的参数转换为网络字节序存储。
     */
    void initialize(uint8_t version, uint8_t traffic_class, uint32_t flow_label,
        uint16_t length, uint8_t next, uint8_t limit,
        const array<uint8_t, 16>& src_addr,
        const array<uint8_t, 16>& dst_addr) {

        // 1. 组合 Version (4), Traffic Class (8), Flow Label (20)
        // 确保 Flow Label 只有 20 位
        if (flow_label > 0xFFFFF) {
            throw invalid_argument("Flow Label exceeds 20 bits.");
        }

        uint32_t combined = (static_cast<uint32_t>(version & 0xF) << 28) |
            (static_cast<uint32_t>(traffic_class & 0xFF) << 20) |
            flow_label;

        // 存储为网络字节序
        this->v_tc_flow = HostToNet32(combined);
        this->payload_length = HostToNet16(length);
        this->next_header = next;
        this->hop_limit = limit;
        this->source_address = src_addr;
        this->destination_address = dst_addr;
    }
};

/**
 * @brief IPv6 扩展头基类
 * * 提供扩展头结构的抽象接口。
 */
struct Ipv6ExtensionHeader {
    // 由于扩展头是变长的，我们不在基类中定义 PACKED 结构体，
    // 而是作为抽象接口，允许具体的扩展头实现其序列化方法。

    /**
     * @brief 获取扩展头在网络中实际占用的总字节数。
     * @return 扩展头的总长度（字节）。
     */
    virtual size_t get_total_length() const = 0;

    /**
     * @brief 将扩展头序列化（写入）到原始字节流中。
     * @param buffer 目标字节向量。
     * @param current_next_header 当前的 Next Header 字段值（作为输入）。
     * @return 序列化后的 Next Header 字段值（作为输出）。
     */
    virtual uint8_t serialize(vector<uint8_t>& buffer, uint8_t current_next_header) const = 0;

    virtual ~Ipv6ExtensionHeader() = default;
};

/**
 * @brief 示例：分段头 (Fragment Header) 的结构和实现
 */
struct FragmentHeaderFields PACKED {
    uint8_t next_header;        // 8 bits
    uint8_t reserved;           // 8 bits (在分段头中，此字段始终为 0)
    uint16_t offset_res_m;     // Fragment Offset (13 bits), Reserved (2 bits), M Flag (1 bit)
    uint32_t identification;    // 32 bits
};

struct Ipv6FragmentHeader : public Ipv6ExtensionHeader {
private:
    uint8_t final_next_header;
    uint16_t fragment_offset; // 主机字节序
    bool more_fragments;      // M Flag
    uint32_t id;              // 主机字节序

public:
    Ipv6FragmentHeader(uint8_t next_hdr, uint16_t offset, bool m_flag, uint32_t identification)
        : final_next_header(next_hdr), fragment_offset(offset), more_fragments(m_flag), id(identification) {
    }

    size_t get_total_length() const override {
        return sizeof(FragmentHeaderFields); // 固定 8 字节
    }

    uint8_t serialize(vector<uint8_t>& buffer, uint8_t current_next_header) const override {
        FragmentHeaderFields hdr;

        // 1. 设置 Next Header
        hdr.next_header = final_next_header; // 这里的 Next Header 指向下一个头或上层协议

        // 2. 设置 Reserved
        hdr.reserved = 0;

        // 3. 组合 Offset (13 bits) 和 M Flag (1 bit)
        uint16_t combined = (fragment_offset << 3) | (more_fragments ? 1 : 0);
        hdr.offset_res_m = HostToNet16(combined);

        // 4. 设置 Identification
        hdr.identification = HostToNet32(id);

        // 5. 将结构体字节复制到缓冲区
        const uint8_t* byte_ptr = reinterpret_cast<const uint8_t*>(&hdr);
        buffer.insert(buffer.end(), byte_ptr, byte_ptr + sizeof(FragmentHeaderFields));

        // 返回当前扩展头的协议号 (Fragment Header Type = 44)
        return 44;
    }
};

/**
 * @brief IPv6 完整包结构体
 * * 提供一个用于构建和序列化完整数据包的容器和方法。
 */
struct Ipv6Packet {
    Ipv6BaseHeader base_header;
    vector<unique_ptr<Ipv6ExtensionHeader>> extension_headers;
    vector<uint8_t> payload;

    /**
     * @brief 接口：添加扩展头
     */
    void add_extension_header(unique_ptr<Ipv6ExtensionHeader> ext_header) {
        extension_headers.push_back(move(ext_header));
    }

    /**
     * @brief 序列化整个 IPv6 包到原始字节流。
     * @return 包含完整 IPv6 包（基本头 + 扩展头 + 负载）的字节向量。
     */
    vector<uint8_t> serialize_packet() {
        uint16_t total_ext_len = 0;

        // 1. 计算所有扩展头总长度并更新 Base Header 的 Next Header 

        // 1.1 累加扩展头长度
        for (const auto& ext : extension_headers) {
            total_ext_len += ext->get_total_length();
        }

        // 1.2 更新 Payload Length: (扩展头长度 + 负载长度)
        uint16_t final_payload_length = total_ext_len + payload.size();
        base_header.payload_length = HostToNet16(final_payload_length);

        // 2. 构建最终的字节序列
        vector<uint8_t> packet_data;

        // 2.1 序列化基本头
        const uint8_t* base_hdr_ptr = reinterpret_cast<const uint8_t*>(&base_header);
        packet_data.insert(packet_data.end(), base_hdr_ptr, base_hdr_ptr + sizeof(Ipv6BaseHeader));

        // 2.2 序列化扩展头并更新 Next Header 链
        // 原始 Base Header 中的 next_header 字段 (上层协议号) 会被移动到最后一个扩展头的 next_header 字段中。
        // Base Header 的 next_header 字段则会指向第一个扩展头的类型（即 serialize 返回的值）。

        // 这里需要更正原始逻辑：Extension Headers 的 Next Header 字段应该在序列化时，
        // 指向下一个扩展头或最终的上层协议。

        // 简单化处理：只更新 Base Header 的 next_header

        // 我们只在第一次循环时，将 Base Header 的 Next Header 字段设置为第一个扩展头类型
        // 并且要求所有扩展头在其构造函数中知道其下一个头部的类型。
        // 此处的 serialize 实现已经包含了将下一个头部类型（final_next_header）写入自身 Next Header 字段的逻辑。

        for (size_t i = 0; i < extension_headers.size(); ++i) {
            // 将扩展头序列化到数据包中
            uint8_t ext_header_type = extension_headers[i]->serialize(packet_data, base_header.next_header);

            // 如果是第一个扩展头，更新 Base Header 的 next_header
            if (i == 0) {
                // **关键步骤：** Base Header 的 Next Header 指向第一个扩展头
                base_header.next_header = ext_header_type;

                // 将更新后的基本头部分写回 packet_data 的起始位置
                copy(reinterpret_cast<const uint8_t*>(&base_header),
                    reinterpret_cast<const uint8_t*>(&base_header) + sizeof(Ipv6BaseHeader),
                    packet_data.begin());
            }
        }

        // 2.3 添加负载
        packet_data.insert(packet_data.end(), payload.begin(), payload.end());

        return packet_data;
    }
};